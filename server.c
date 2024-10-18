#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>

#include <sys/types.h>
#include <errno.h>

#define MAX_SIZE 1024
#define MAX_STORAGE 50 * 1024

#define MAX_FILES 100
#define MAX_FILENAME_SIZE 100

void createUser(int clientSocket);
void authenticateUser(int clientSocket);
int parseFileAfterAsterisk(const char *userName, char fileNames[MAX_FILES][MAX_FILENAME_SIZE], int *fileCount);
void write_FileInfo_to_user_Config(int clientSocket, const char *userName, const char *fileName, size_t fileSize);
int viewFile(int clientSocket, const char *userName);
//void handleFileExists(int clientSocket, const char *fileName, const char *userName);
void processFileManagement(int clientSocket, const char *userName);
void *handleClient(void *clientSocketPtr);
void receive_updated_file_content(int clientSocket, const char *userName);

/* =====================================================================
                        Helper Functions
========================================================================  */

int calculateSumOfSizes(int *sizes, int count)
{
    int sum = 0;
    for (int i = 0; i < count; i++)
    {
        sum += sizes[i];
    }
    return sum;
}

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

/* =====================================================================
                Send and Recieve File to the Client and View Files
   =====================================================================  */
void sendFileToClient(int clientSocket, const char *userName)
{
    char fileName[256];

    ssize_t fileNameSize = recv(clientSocket, fileName, sizeof(fileName) - 1, 0);
    if (fileNameSize <= 0)
    {
        perror("Error receiving file name");
        close(clientSocket);
        return;
    }
    fileName[fileNameSize] = '\0';

    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s", userName, fileName);

    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        send(clientSocket, "No file found", 13, 0);
        perror("File not found");
        close(clientSocket);
        return;
    }

    send(clientSocket, "$READY$", 7, 0);

    char buffer[MAX_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        ssize_t sentBytes = send(clientSocket, buffer, bytesRead, 0);
        if (sentBytes < 0)
        {
            perror("Error sending file data");
            break;
        }
    }

    printf("File sent successfully: %s\n", filePath);
    fclose(file);
}

void receiveFileFromClient(int clientSocket, const char *userName)
{
    char fileName[256];

    ssize_t fileNameSize = recv(clientSocket, fileName, sizeof(fileName) - 1, 0);
    if (fileNameSize <= 0)
    {
        perror("Error receiving file name");
        close(clientSocket);
        return;
    }
    fileName[fileNameSize] = '\0';

    printf("Receiving file: %s\n", fileName);

    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s", userName, fileName);

    FILE *encoded_file = fopen(filePath, "w");
    if (encoded_file == NULL)
    {
        perror("Error creating file");
        close(clientSocket);
        return;
    }

    send(clientSocket, "$READY$", 7, 0);

    char buffer[1024];
    ssize_t bytesRead;
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
    {
        fwrite(buffer, sizeof(char), bytesRead, encoded_file);
    }

    if (bytesRead < 0)
    {
        perror("Error receiving file data");
    }
    else
    {
        printf("File received and saved successfully in %s.\n", filePath);
        send(clientSocket, "$SUCCESS$", 9, 0);
    }

    fclose(encoded_file);
}

/* =====================================================================
                        View Files Functionality
========================================================================  */

int viewFile(int clientSocket, const char *userName)
{
    char user_config[1024];
    // snprintf(user_config, sizeof(user_config), "%s.config", userName);
    snprintf(user_config, sizeof(user_config), "%s/%s.config", userName, userName);

    int fileDescriptor = open(user_config, O_RDONLY);
    if (fileDescriptor < 0)
    {
        perror("Error opening config file");
        return -1;
    }

    char view_buffer[MAX_SIZE];
    ssize_t bytesReads;
    int foundAsterisk = 0;

    while ((bytesReads = read(fileDescriptor, view_buffer, sizeof(view_buffer) - 1)) > 0)
    {
        view_buffer[bytesReads] = '\0';

        if (!foundAsterisk)
        {
            char *asteriskPos = strchr(view_buffer, '*');
            if (asteriskPos != NULL)
            {
                foundAsterisk = 1;
                char *newlinePos = strchr(asteriskPos, '\n');
                if (newlinePos != NULL)
                {
                    newlinePos++;
                    memmove(view_buffer, newlinePos, bytesReads - (newlinePos - view_buffer));
                    bytesReads -= (newlinePos - view_buffer);
                    view_buffer[bytesReads] = '\0';
                }
                else
                {
                    continue;
                }
            }
        }

        if (foundAsterisk)
        {
            send(clientSocket, view_buffer, bytesReads, 0);
            printf("%s", view_buffer);
        }
    }

    if (bytesReads < 0)
    {
        perror("Error reading config file");
        close(fileDescriptor);
        return -2;
    }

    printf("\nData about Files sent successfully \n");
    close(fileDescriptor);
    return 0;
}

/* =====================================================================
       User file storage checking Files Functionality
========================================================================  */

void handleFileExists(int clientSocket, const char *fileName, const char *userName)
{
    const char *fileExistsMsg = "File already exists.";
    send(clientSocket, fileExistsMsg, strlen(fileExistsMsg), 0);

    // Receive the client's response (0 or 1)
    char clientResponse[2];
    recv(clientSocket, clientResponse, 2, 0);

    if (clientResponse[0] == '1')
    {
        // write your code here for replacing the file
        // printf("File '%s' will be replaced for user: %s\n", fileName, userName);
        printf("File '%s' replaced for user: %s\n", fileName, userName);
        receive_updated_file_content(clientSocket, userName);
        return;
    }
    else if (clientResponse[0] == '0')
    {
        // write your code here for not replacing the file
        printf("File '%s' will not be replaced for user: %s\n", fileName, userName);
    }
    else
    {
        printf("Invalid input from client.\n");
    }
}

void write_FileInfo_to_user_Config(int clientSocket, const char *userName, const char *fileName, size_t fileSize)
{
    char fileNames[MAX_FILES][MAX_FILENAME_SIZE];
    int fileCount = 0;

    int totalSize = parseFileAfterAsterisk(userName, fileNames, &fileCount);

    if (totalSize < 0)
    {
        fprintf(stderr, "Error parsing config file. Error code: %d\n", totalSize);
        const char *errorMsg = "Error parsing config file.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    printf("Total size: %d\n", totalSize);

    if (totalSize + fileSize > MAX_STORAGE)
    {
        const char *outOfSpaceMsg = "Out of space.";
        send(clientSocket, outOfSpaceMsg, strlen(outOfSpaceMsg), 0);
        printf("Out of Space\n");
        return;
    }

    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s.config", userName, userName);

    char fileListPath[1024];
    snprintf(fileListPath, sizeof(fileListPath), "%s/%s_fileList.config", userName, userName);

    char userFilePath[1024];
    snprintf(userFilePath, sizeof(userFilePath), "%s/%s", userName, fileName);

    if (access(userFilePath, F_OK) == 0)
    {
        // const char *fileExistsMsg = "File already exists.";
        // send(clientSocket, fileExistsMsg, strlen(fileExistsMsg), 0);
        // printf("File '%s' already exists for user: %s\n", fileName, userName);
        // return;
        handleFileExists(clientSocket, filePath, userName);
        return;
    }

    int fileDescriptor = open(filePath, O_WRONLY | O_APPEND);
    if (fileDescriptor < 0)
    {
        perror("Error opening config file for appending");
        const char *errorMsg = "Error updating config file.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    if (dprintf(fileDescriptor, "%s - %zu\n", fileName, fileSize) < 0)
    {
        perror("Error writing to config file");
        const char *errorMsg = "Error writing to config file.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
    }
    else
    {
        const char *successMsg = "File Data updated.";
        send(clientSocket, successMsg, strlen(successMsg), 0);
        printf("File Data updated for user: %s\n", userName);
    }

    close(fileDescriptor);

    // Write to the fileList.config file
    int fileListDescriptor = open(fileListPath, O_WRONLY | O_APPEND);
    if (fileListDescriptor < 0)
    {
        perror("Error opening file list for appending");
        const char *errorMsg = "Error updating file list.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    if (dprintf(fileListDescriptor, "%s - 1\n", fileName) < 0)
    {
        perror("Error writing to file list");
        const char *errorMsg = "Error writing to file list.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
    }
    else
    {
        printf("FileList updated with '%s - 1' for user: %s\n", fileName, userName);
    }

    close(fileListDescriptor);

    receiveFileFromClient(clientSocket, userName);
}

/* =====================================================================
                       Authentication Step
========================================================================  */

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
        send(clientSocket, noFIleFound, strlen(noFIleFound), 0);
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
        const char *successMsg = "User found\0";
        send(clientSocket, successMsg, strlen(successMsg), 0);
        printf("User %s authenticated\n", userName);

        processFileManagement(clientSocket, userName);
    }
    else
    {
        const char *errorMessage = "Incorrect password\0";
        send(clientSocket, errorMessage, strlen(errorMessage), 0);

        printf("Incorrect password for user %s\n", userName);
    }
}

/* =====================================================================
            Parse File for Checking User's Storage And Info
========================================================================  */

int parseFileAfterAsterisk(const char *userName, char fileNames[MAX_FILES][MAX_FILENAME_SIZE], int *fileCount)
{
    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s.config", userName, userName);

    int fileDescriptor = open(filePath, O_RDONLY);
    if (fileDescriptor < 0)
    {
        perror("Error opening config file");
        return -1;
    }

    char buffer[MAX_SIZE];
    ssize_t bytesRead;
    int foundAsterisk = 0;

    int fileSizes[MAX_FILES];
    *fileCount = 0;

    while ((bytesRead = read(fileDescriptor, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytesRead] = '\0';

        if (!foundAsterisk)
        {
            char *asteriskPos = strchr(buffer, '*');
            if (asteriskPos != NULL)
            {
                foundAsterisk = 1;
                char *newlinePos = strchr(asteriskPos, '\n');
                if (newlinePos != NULL)
                {
                    newlinePos++;
                    memmove(buffer, newlinePos, bytesRead - (newlinePos - buffer));
                    bytesRead -= (newlinePos - buffer);
                    buffer[bytesRead] = '\0';
                }
            }
        }

        if (foundAsterisk)
        {
            char *linePtr = buffer;
            while (*linePtr != '\0' && *fileCount < MAX_FILES)
            {
                char *spacePtr = strchr(linePtr, ' ');
                if (spacePtr == NULL)
                {
                    break;
                }

                *spacePtr = '\0';
                strncpy(fileNames[*fileCount], linePtr, MAX_FILENAME_SIZE - 1);
                fileNames[*fileCount][MAX_FILENAME_SIZE - 1] = '\0';

                linePtr = spacePtr + 1;

                char *dashPtr = strchr(linePtr, '-');
                if (dashPtr == NULL)
                {
                    break;
                }

                linePtr = dashPtr + 1;
                while (*linePtr == ' ')
                    linePtr++;

                char *endPtr;
                fileSizes[*fileCount] = (int)strtol(linePtr, &endPtr, 10);

                if (endPtr == linePtr || fileSizes[*fileCount] <= 0)
                {
                    fprintf(stderr, "Error parsing file size\n");
                    close(fileDescriptor);
                    return -5;
                }

                (*fileCount)++;

                linePtr = strchr(endPtr, '\n');
                if (linePtr != NULL)
                {
                    linePtr++;
                }
                else
                {
                    break;
                }
            }
        }
    }

    if (bytesRead < 0)
    {
        perror("Error reading config file");
        close(fileDescriptor);
        return -2;
    }

    close(fileDescriptor);

    printf("Files and Sizes:\n");
    for (int i = 0; i < *fileCount; i++)
    {
        printf("File: %s\n", fileNames[i]);
    }

    int totalSize = calculateSumOfSizes(fileSizes, *fileCount);
    printf("\nTotal Size of All Files: %d bytes\n", totalSize);

    return totalSize;
}

/* =====================================================================
            Delete file req from user
========================================================================  */

void delete_File_from_user_Config(int clientSocket, const char *userName, const char *fileName)
{
    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s.config", userName, userName);

    char userFilePath[1024];
    snprintf(userFilePath, sizeof(userFilePath), "%s/%s", userName, fileName);

    if (access(userFilePath, F_OK) != 0)
    {
        const char *fileNotFoundMsg = "File not found.";
        send(clientSocket, fileNotFoundMsg, strlen(fileNotFoundMsg), 0);
        printf("File '%s' not found for user: %s\n", fileName, userName);
        return;
    }

    const char *fileFoundMsg = "File is found!!";
    send(clientSocket, fileFoundMsg, strlen(fileFoundMsg), 0);
    printf("File '%s' found for user: %s\n", fileName, userName);

    FILE *configFile = fopen(filePath, "r");
    if (!configFile)
    {
        perror("Error opening config file");
        const char *errorMsg = "Error opening config file.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    char password[256];
    char line[1024];
    char fileNames[MAX_FILES][MAX_FILENAME_SIZE];
    int fileSizes[MAX_FILES];
    int fileCount = 0;
    int fileIndex = -1;

    if (fgets(password, sizeof(password), configFile) == NULL)
    {
        perror("Error reading password from config file");
        fclose(configFile);
        const char *errorMsg = "Error reading config file.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    if (fgets(line, sizeof(line), configFile) == NULL || strcmp(line, "*\n") != 0)
    {
        perror("Error reading asterisk line from config file");
        fclose(configFile);
        const char *errorMsg = "Error reading config file.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    while (fgets(line, sizeof(line), configFile) != NULL)
    {
        char tempFileName[MAX_FILENAME_SIZE];
        int tempFileSize;

        if (sscanf(line, "%s - %d", tempFileName, &tempFileSize) == 2)
        {
            strcpy(fileNames[fileCount], tempFileName);
            fileSizes[fileCount] = tempFileSize;

            if (strcmp(tempFileName, fileName) == 0)
            {
                fileIndex = fileCount;
            }

            fileCount++;
        }
    }

    fclose(configFile);

    if (fileIndex == -1)
    {
        const char *fileNotFoundInConfig = "File not found in config.";
        send(clientSocket, fileNotFoundInConfig, strlen(fileNotFoundInConfig), 0);
        printf("File '%s' not found in config for user: %s\n", fileName, userName);
        return;
    }

    if (remove(userFilePath) != 0)
    {
        perror("Error deleting file from user's folder");
        const char *deleteError = "Error deleting file.";
        send(clientSocket, deleteError, strlen(deleteError), 0);
        return;
    }

    FILE *configFileWrite = fopen(filePath, "w");
    if (!configFileWrite)
    {
        perror("Error opening config file for writing");
        const char *errorMsg = "Error updating config file.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    fprintf(configFileWrite, "%s", password);
    fprintf(configFileWrite, "*\n");

    for (int i = 0; i < fileCount; i++)
    {
        if (i != fileIndex)
        {
            fprintf(configFileWrite, "%s - %d\n", fileNames[i], fileSizes[i]);
        }
    }

    fclose(configFileWrite);

    const char *successMsg = "File successfully deleted.";
    send(clientSocket, successMsg, strlen(successMsg), 0);
    printf("File '%s' successfully deleted from user: %s\n", fileName, userName);
}

/* =====================================================================
                 Update the file requested from user
========================================================================  */
void receive_updated_file_content(int clientSocket, const char *userName)
{
    char fileName[256];
    ssize_t fileNameSize = recv(clientSocket, fileName, sizeof(fileName) - 1, 0);
    if (fileNameSize <= 0)
    {
        perror("Error receiving file name");
        close(clientSocket);
        return;
    }
    fileName[fileNameSize] = '\0';

    printf("Receiving file: %s\n", fileName);
    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s", userName, fileName);

    FILE *encoded_file = fopen(filePath, "w+");
    if (encoded_file == NULL)
    {
        perror("Error creating or opening file");
        close(clientSocket);
        return;
    }
    send(clientSocket, "$READY$", 7, 0);
    char buffer[1024];
    ssize_t bytesRead;
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
    {
        fwrite(buffer, sizeof(char), bytesRead, encoded_file);
    }

    if (bytesRead < 0)
    {
        perror("Error receiving file data");
    }
    else
    {
        printf("File received and saved successfully in %s.\n", filePath);
        send(clientSocket, "$SUCCESS$", 9, 0);
    }
    fclose(encoded_file);
}

/* =====================================================================
            handle download and Upload Signal from client
========================================================================  */

void processFileManagement(int clientSocket, const char *userName)
{
    int option;
    ssize_t bytesReceived;

    bytesReceived = recv(clientSocket, &option, sizeof(option), 0);
    if (bytesReceived <= 0)
    {
        perror("Error receiving option");
        return;
    }

    if (option == 1)
    {
        char fileName[MAX_SIZE];
        long fileSize;

        bytesReceived = recv(clientSocket, fileName, sizeof(fileName) - 1, 0);
        if (bytesReceived <= 0)
        {
            perror("Error receiving file name");
            return;
        }
        fileName[bytesReceived] = '\0';

        bytesReceived = recv(clientSocket, (char *)&fileSize, sizeof(fileSize), 0);

        if (bytesReceived <= 0)
        {
            perror("Error receiving file size");
            return;
        }

        write_FileInfo_to_user_Config(clientSocket, userName, fileName, fileSize);
        // receiveFileFromClient(clientSocket,userName);
    }
    else if (option == 2)
    {
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
            send(clientSocket, fileFoundMsg, strlen(fileFoundMsg), 0);
            sendFileToClient(clientSocket, userName);
        }
        else
        {
            printf("The file '%s' does not exist in the list.\n", fileName);
            const char *errorMsg = "Error parsing config file.";
            send(clientSocket, errorMsg, strlen(errorMsg), 0);
        }
    }
    else if (option == 3)
    {
        viewFile(clientSocket, userName);
    }
    else if (option == 4)
    {
        char fileName[MAX_FILENAME_SIZE];

        bytesReceived = recv(clientSocket, fileName, sizeof(fileName) - 1, 0);
        if (bytesReceived <= 0)
        {
            perror("Error receiving file name for download");
            return;
        }
        fileName[bytesReceived] = '\0';
        delete_File_from_user_Config(clientSocket, userName, fileName);
    }

    else if (option == 5)
    {
        receive_updated_file_content(clientSocket, userName);
    }
}

/* =====================================================================
            handle client for authentication signals
========================================================================  */

void *handleClient(void *clientSocketPtr)
{
    int clientSocket = *((int *)clientSocketPtr);
    free(clientSocketPtr);

    int option;
    ssize_t bytesReceived;

    bytesReceived = recv(clientSocket, &option, sizeof(option), 0);
    if (bytesReceived <= 0)
    {
        perror("Error receiving option");
        close(clientSocket);
        return NULL;
    }

    if (option == 1)
    {
        createUser(clientSocket);
    }
    else if (option == 2)
    {
        authenticateUser(clientSocket);
    }
    else
    {
        send(clientSocket, "Invalid option", strlen("Invalid option"), 0);
    }

    close(clientSocket);
    return NULL;
}

int main()
{
    int serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8081);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Error binding socket");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 5) < 0)
    {
        perror("Error listening");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port 8081...\n");

    while (1)
    {
        int *clientSocket = malloc(sizeof(int));
        if (!clientSocket)
        {
            perror("Error allocating memory");
            continue;
        }

        *clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (*clientSocket < 0)
        {
            perror("Error accepting connection");
            free(clientSocket);
            continue;
        }

        printf("Client connected\n");

        pthread_t clientThread;
        if (pthread_create(&clientThread, NULL, handleClient, clientSocket) != 0)
        {
            perror("Error creating thread");
            close(*clientSocket);
            free(clientSocket);
            continue;
        }

        pthread_detach(clientThread);
    }

    close(serverSocket);
    return 0;
}
