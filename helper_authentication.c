#include "helper.h"

/*   handle download and Upload Signal from client  */

int checkFileExists(char fileNames[MAX_FILES][MAX_FILENAME_SIZE], int fileCount, const char *inputFileName)
{
    for (int i = 0; i < fileCount; i++)
    {
        if (strcmp(fileNames[i], inputFileName) == 0)
        {
            return 1;
        }
    }
    return 0;
}

void process_task_managment(int clientSocket, const char *userName)
{
    int option;
    ssize_t bytesReceived;
    bytesReceived = recv(clientSocket, &option, sizeof(option), 0);
    if (bytesReceived <= 0)
    {
        perror("Error receiving option");
        return;
    }

    // Upload  FileCode
    if (option == 1)
    {
        writer_entry();
        char buffer[1024];
        char fileName[256];
        long fileSize;
        char fileSizeStr[50];

        bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0)
        {
            perror("Error receiving data");
            return;
        }
        buffer[bytesReceived] = '\0';

        // Signal to seperate Name and Size of file
        char *delimiterPos = strchr(buffer, '\n');
        if (delimiterPos == NULL)
        {
            perror("Error: No delimiter found between file name and file size");
            return;
        }

        strncpy(fileName, buffer, delimiterPos - buffer);
        fileName[delimiterPos - buffer] = '\0';
        strcpy(fileSizeStr, delimiterPos + 1);

        printf("File \' %s \' with size %s Received from User \"%s\"\n", fileName, fileSizeStr, userName);
        // printf("File size: %s bytes\n", fileSizeStr);

        fileSize = strtol(fileSizeStr, NULL, 10);
        if (fileSize == 0 && (fileSizeStr[0] != '0' || fileSizeStr[0] == '\0'))
        {
            perror("Error converting file size to long");
            return;
        }

        printf("File size Recieved: %ld bytes\n", fileSize);
        write_FileInfo_to_user_Config(clientSocket, userName, fileName, fileSize);
        writer_exit();
    }
    // Download  FileCode
    else if (option == 2)
    {
        reader_entry();
        char fileName[MAX_FILENAME_SIZE];
        bytesReceived = recv(clientSocket, fileName, sizeof(fileName) - 1, 0);
        if (bytesReceived <= 0)
        {
            perror("Error receiving file name for download");
            return;
        }
        fileName[bytesReceived] = '\0';

        char fileNames[MAX_FILES][MAX_FILENAME_SIZE];
        int fileCount = 0;

        parseFileAfterAsterisk(userName, fileNames, &fileCount);
        if (checkFileExists(fileNames, fileCount, fileName))
        {
            printf("The file '%s' exists in the list.\n", fileName);
            const char *fileFoundMsg = "File found.";
            send(clientSocket, fileFoundMsg, strlen(fileFoundMsg) + 1, 0);
            sendFileToClient(clientSocket, userName);
        }
        else
        {
            printf("The file '%s' does not exist in the list.\n", fileName);
            const char *errorMsg = "Error parsing config file.";
            send(clientSocket, errorMsg, strlen(errorMsg) + 1, 0);
        }
        reader_exit();
    }
    // View Files Code
    else if (option == 3)
    {
        reader_entry();
        viewFile(clientSocket, userName);
        reader_exit();
    }
    // Delete File Code
    else if (option == 4)
    {
        writer_entry();
        char fileName[MAX_FILENAME_SIZE];
        bytesReceived = recv(clientSocket, fileName, sizeof(fileName) - 1, 0);
        if (bytesReceived <= 0)
        {
            perror("Error receiving file name for download");
            return;
        }
        fileName[bytesReceived] = '\0';
        delete_File_from_user_Config(clientSocket, userName, fileName);
        writer_exit();
    }
    // Update File Code
    else if (option == 5)
    {
        writer_entry();
        receive_updated_file_content(clientSocket, userName);
        writer_exit();
    }
}

/*              Authentication Step  */
void createUser(int clientSocket)
{
    char userName[256];

    ssize_t userNameSize = recv(clientSocket, userName, sizeof(userName) - 1, 0);
    if (userNameSize <= 0)
    {
        perror("Error receiving username");
        return;
    }
    userName[userNameSize] = '\0';

    int userExists = 0;

    if (mkdir(userName, 0755) < 0)
    {
        if (errno == EEXIST)
        {
            userExists = 0;
        }
        else
        {
            perror("Error creating user directory");
            return;
        }
    }
    else
    {
        userExists = 1;
    }

    send(clientSocket, &userExists, sizeof(userExists), 0);

    if (userExists == 0)
    {
        printf("User directory already exists: %s\n", userName);
        return;
    }

    char filePath[1024];
    char fileListPath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s.config", userName, userName);
    snprintf(fileListPath, sizeof(fileListPath), "%s/%s_fileList.config", userName, userName);

    int fileDescriptor = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fileDescriptor < 0)
    {
        perror("Error creating config file");
        return;
    }

    int fileListDescriptor = open(fileListPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fileListDescriptor < 0)
    {
        perror("Error creating file list");
        close(fileDescriptor);
        return;
    }

    char password[MAX_SIZE];
    ssize_t passwordSize = recv(clientSocket, password, sizeof(password) - 1, 0);
    if (passwordSize <= 0)
    {
        perror("Error receiving password");
        close(fileDescriptor);
        close(fileListDescriptor);
        return;
    }
    password[passwordSize] = '\0';

    dprintf(fileDescriptor, "%s\n*\n", password);

    close(fileDescriptor);
    close(fileListDescriptor);

    printf("User registered with name: %s\n", userName);
}

void authenticateUser(int clientSocket)
{
    char userName[256];
    ssize_t userNameSize = recv(clientSocket, userName, sizeof(userName) - 1, 0);
    if (userNameSize <= 0)
    {
        perror("Error receiving username");
        return;
    }
    userName[userNameSize] = '\0';

    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s.config", userName, userName);

    if (access(filePath, F_OK) != 0)
    {
        const char *noFIleFound = "No file found, no user registered\0";
        send(clientSocket, noFIleFound, strlen(noFIleFound) + 1, 0);
        return;
    }

    int fileDescriptor = open(filePath, O_RDONLY);
    if (fileDescriptor < 0)
    {
        perror("Error opening config file");
        return;
    }

    char storedPassword[MAX_SIZE];
    ssize_t bytesRead = read(fileDescriptor, storedPassword, sizeof(storedPassword) - 1);
    if (bytesRead <= 0)
    {
        perror("Error reading password");
        close(fileDescriptor);
        return;
    }
    storedPassword[bytesRead] = '\0';

    char *passwordEnd = strchr(storedPassword, '*');
    if (passwordEnd != NULL)
    {
        *passwordEnd = '\0';
    }
    size_t len = strlen(storedPassword);
    if (len > 0 && storedPassword[len - 1] == '\n')
    {
        storedPassword[len - 1] = '\0';
    }
    close(fileDescriptor);

    char inputPassword[MAX_SIZE];
    ssize_t passwordSize = recv(clientSocket, inputPassword, sizeof(inputPassword) - 1, 0);
    if (passwordSize <= 0)
    {
        perror("Error receiving password");
        close(clientSocket);
        return;
    }
    inputPassword[passwordSize] = '\0';

    if (strcmp(storedPassword, inputPassword) == 0)
    {
        const char *successMsg = "User found";
        send(clientSocket, successMsg, strlen(successMsg) + 1, 0);
        printf("User %s authenticated\n", userName);

        process_task_managment(clientSocket, userName);
    }
    else
    {
        const char *errorMessage = "Incorrect password\0";
        send(clientSocket, errorMessage, strlen(errorMessage) + 1, 0);

        printf("Incorrect password for user %s\n", userName);
    }
}
