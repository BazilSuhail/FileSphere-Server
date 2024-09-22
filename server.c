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
#define MAX_STORAGE 50

#define MAX_FILES 100
#define MAX_FILENAME_SIZE 100

void createUser(int clientSocket);
void authenticateUser(int clientSocket);
int parseFileAfterAsterisk(const char *userName, char fileNames[MAX_FILES][MAX_FILENAME_SIZE], int *fileCount);
void write_FileInfo_to_user_Config(int clientSocket, const char *userName, const char *fileName, size_t fileSize);
int viewFile(int clientSocket, const char *userName);
void processFileManagement(int clientSocket, const char *userName);
void *handleClient(void *clientSocketPtr);

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

    // Receive the file name from the client
    ssize_t fileNameSize = recv(clientSocket, fileName, sizeof(fileName) - 1, 0);
    if (fileNameSize <= 0)
    {
        perror("Error receiving file name");
        close(clientSocket);
        return;
    }
    fileName[fileNameSize] = '\0';

    // Construct the file path inside the user's folder
    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s", userName, fileName); // Path: userName/fileName

    // Check if the file exists in the user's folder
    int fileDescriptor = open(filePath, O_RDONLY);
    if (fileDescriptor < 0)
    {
        send(clientSocket, "No file found", 13, 0);
        perror("File not found");
        close(clientSocket);
        return;
    }

    // Send readiness signal to the client
    send(clientSocket, "$READY$", 7, 0);

    // Send file data
    char buffer[MAX_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = read(fileDescriptor, buffer, sizeof(buffer))) > 0)
    {
        send(clientSocket, buffer, bytesRead, 0);
    }

    // Log the file transfer success
    printf("File sent successfully: %s\n", filePath);

    // Close the file descriptor
    close(fileDescriptor);
}

void receiveFileFromClient(int clientSocket, const char *userName)
{
    char fileName[256];

    // Receive the file name from the client
    ssize_t fileNameSize = recv(clientSocket, fileName, sizeof(fileName) - 1, 0);
    if (fileNameSize <= 0)
    {
        perror("Error receiving file name");
        close(clientSocket);
        return;
    }
    fileName[fileNameSize] = '\0';

    printf("Receiving file: %s\n", fileName);

    // Construct the file path inside the user's folder
    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s", userName, fileName); // Path: userName/fileName

    // Create or overwrite the file in the user's folder with write permissions
    int fileDescriptor = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fileDescriptor < 0)
    {
        perror("Error creating file");
        close(clientSocket);
        return;
    }

    // Send readiness signal to the client
    send(clientSocket, "$READY$", 7, 0);

    // Receive file data and write it to the file
    char buffer[MAX_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
    {
        if (write(fileDescriptor, buffer, bytesRead) != bytesRead)
        {
            perror("Error writing to file");
            close(fileDescriptor);
            close(clientSocket);
            return;
        }
    }

    // Check if there was an error receiving data
    if (bytesRead < 0)
    {
        perror("Error receiving file data");
    }
    else
    {
        printf("File received and saved successfully in %s.\n", filePath);
        send(clientSocket, "$SUCCESS$", 9, 0);
    }

    // Close the file descriptor
    close(fileDescriptor);
}

/* =====================================================================
                        View Files Functionality
========================================================================  */

int viewFile(int clientSocket, const char *userName)
{
    char user_config[1024];
    // snprintf(user_config, sizeof(user_config), "%s.config", userName);

    snprintf(user_config, sizeof(user_config), "%s/%s.config", userName, userName); // Path: userName/fileName

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
                        View Files Functionality
========================================================================  */

void write_FileInfo_to_user_Config(int clientSocket, const char *userName, const char *fileName, size_t fileSize)
{
    char fileNames[MAX_FILES][MAX_FILENAME_SIZE]; // Array to store file names
    int fileCount = 0;

    // Parse the existing file data after the asterisk in the config file
    int totalSize = parseFileAfterAsterisk(userName, fileNames, &fileCount);

    if (totalSize < 0)
    {
        fprintf(stderr, "Error parsing config file. Error code: %d\n", totalSize);
        const char *errorMsg = "Error parsing config file.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    printf("Total size: %d\n", totalSize);

    // Check if adding the new file exceeds the maximum storage limit
    if (totalSize + fileSize > MAX_STORAGE)
    {
        const char *outOfSpaceMsg = "Out of space.";
        send(clientSocket, outOfSpaceMsg, strlen(outOfSpaceMsg), 0);
        printf("Out of Space\n");
        return;
    }

    // Create the correct file path for the user's config file inside their directory
    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s.config", userName, userName); // Path: username/username.config

    // Check if the file already exists in the user's folder
    char userFilePath[1024];
    snprintf(userFilePath, sizeof(userFilePath), "%s/%s", userName, fileName); // Path: username/fileName

    if (access(userFilePath, F_OK) == 0)
    {
        // File already exists
        const char *fileExistsMsg = "File already exists.";
        send(clientSocket, fileExistsMsg, strlen(fileExistsMsg), 0);
        printf("File '%s' already exists for user: %s\n", fileName, userName);
        return;
    }

    // Open the user's config file in append mode to add new file info
    int fileDescriptor = open(filePath, O_WRONLY | O_APPEND);
    if (fileDescriptor < 0)
    {
        perror("Error opening config file for appending");
        const char *errorMsg = "Error updating config file.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    // Write the new file information to the config file (fileName - fileSize)
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

    // Close the file descriptor after updating the config file
    close(fileDescriptor);

    receiveFileFromClient(clientSocket, userName);
}

/* =====================================================================
                       Authentication Step
========================================================================  */

void createUser(int clientSocket)
{
    char userName[256]; // Buffer for storing the username

    // Receive the username from the client
    ssize_t userNameSize = recv(clientSocket, userName, sizeof(userName) - 1, 0);
    if (userNameSize <= 0)
    {
        perror("Error receiving username");
        return;
    }
    userName[userNameSize] = '\0'; // Null-terminate the username string

    int userExists = 0; // Variable to hold the status of the folder (0 = exists, 1 = new folder created)

    // Create a directory with the username as its name
    if (mkdir(userName, 0755) < 0) // 0755 sets directory permissions
    {
        if (errno == EEXIST) // If the directory already exists, set userExists to 0
        {
            userExists = 0;
        }
        else // If there's another error, print the error and return
        {
            perror("Error creating user directory");
            return;
        }
    }
    else
    {
        userExists = 1; // If the folder was created, set userExists to 1
    }

    // Send the userExists signal to the client
    send(clientSocket, &userExists, sizeof(userExists), 0);

    // If the folder already exists, we stop further actions
    if (userExists == 0)
    {
        printf("User directory already exists: %s\n", userName);
        return;
    }

    // Create the path for the config file inside the user's directory
    char filePath[1024];                                                      // Use a larger buffer for file path
    snprintf(filePath, sizeof(filePath), "%s/%s.config", userName, userName); // Create the config file path

    // Open the file for writing inside the user's directory
    int fileDescriptor = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fileDescriptor < 0) // Handle error if the file can't be created
    {
        perror("Error creating config file");
        return;
    }

    // Notify the client that the user was created successfully
    // send(clientSocket, "User created", strlen("User created"), 0);

    // Receive the password from the client
    char password[MAX_SIZE]; // Buffer for the password
    ssize_t passwordSize = recv(clientSocket, password, sizeof(password) - 1, 0);
    if (passwordSize <= 0)
    {
        perror("Error receiving password");
        close(fileDescriptor); // Close the file
        return;
    }
    password[passwordSize] = '\0'; // Null-terminate the password

    // Write the password to the config file
    dprintf(fileDescriptor, "%s\n*\n", password);
    printf("User registered with name: %s\n", userName); // Output a message on the server

    // Close the file and client socket
    close(fileDescriptor);
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
    userName[userNameSize] = '\0'; // Null-terminate the username string

    // Create the file path for the config file inside the user's directory
    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s.config", userName, userName); // Path: username/username.config

    // Check if the config file exists in the user's folder
    if (access(filePath, F_OK) != 0)
    {
        const char *noFIleFound="No file found, no user registered";
        send(clientSocket, noFIleFound, strlen(noFIleFound), 0);
        return;
    }

    // Open the config file for reading
    int fileDescriptor = open(filePath, O_RDONLY);
    if (fileDescriptor < 0)
    {
        perror("Error opening config file");
        return;
    }

    // Read the stored password from the config file
    char storedPassword[MAX_SIZE];
    ssize_t bytesRead = read(fileDescriptor, storedPassword, sizeof(storedPassword) - 1);
    if (bytesRead <= 0)
    {
        perror("Error reading password");
        close(fileDescriptor);
        return;
    }
    storedPassword[bytesRead] = '\0'; // Null-terminate the stored password

    // Remove the '*' character and newline from the stored password
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
    close(fileDescriptor); // Close the config file after reading

    // Receive the input password from the client
    char inputPassword[MAX_SIZE];
    ssize_t passwordSize = recv(clientSocket, inputPassword, sizeof(inputPassword) - 1, 0);
    if (passwordSize <= 0)
    {
        perror("Error receiving password");
        close(clientSocket);
        return;
    }
    inputPassword[passwordSize] = '\0'; // Null-terminate the input password

    // Compare the stored password with the received password
    if (strcmp(storedPassword, inputPassword) == 0)
    {
        const char *successMsg = "User found";
        send(clientSocket, successMsg, strlen(successMsg), 0);
        // send(clientSocket, "User found", strlen("User found"), 0);
        printf("User %s authenticated\n", userName);

        // Proceed to the file management function
        processFileManagement(clientSocket, userName);
    }
    else
    {
        const char *errorMessage = "Incorrect password";
        send(clientSocket, errorMessage, strlen(errorMessage), 0);

        // send(clientSocket, "Incorrect password", strlen("Incorrect password"), 0);
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

    int fileSizes[MAX_FILES]; // Array to store file sizes
    *fileCount = 0;           // Initialized file count

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
        // Handle file upload
        char fileName[MAX_SIZE];
        size_t fileSize;

        // Receive file name
        bytesReceived = recv(clientSocket, fileName, sizeof(fileName) - 1, 0);
        if (bytesReceived <= 0)
        {
            perror("Error receiving file name");
            return;
        }
        fileName[bytesReceived] = '\0';

        bytesReceived = recv(clientSocket, &fileSize, sizeof(fileSize), 0);
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
        // Handle file download
        char fileName[MAX_FILENAME_SIZE];

        // Receive the file name from the client
        bytesReceived = recv(clientSocket, fileName, sizeof(fileName) - 1, 0);
        if (bytesReceived <= 0)
        {
            perror("Error receiving file name for download");
            return;
        }
        fileName[bytesReceived] = '\0'; // Null-terminate the file name

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

    // Receive the option from the client
    bytesReceived = recv(clientSocket, &option, sizeof(option), 0);
    if (bytesReceived <= 0)
    {
        perror("Error receiving option");
        close(clientSocket);
        return NULL; // Return NULL instead of void
    }

    // Perform the appropriate action based on the option
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

    close(clientSocket); // Close the client socket after handling the client
    return NULL;         // Return NULL to match the expected return type
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
            close(*clientSocket); // Close client socket on failure
            free(clientSocket);   // Free memory on failure
            continue;
        }

        // Detach the thread so it cleans up automatically
        pthread_detach(clientThread);
    }

    close(serverSocket);
    return 0;
}
