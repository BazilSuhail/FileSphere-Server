#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define MAX_SIZE 1024
#define MAX_STORAGE 50

#define MAX_FILES 100
#define MAX_FILENAME_SIZE 100
 
void createUser(int clientSocket);
void authenticateUser(int clientSocket);
int parseFileAfterAsterisk(const char *userName, char fileNames[MAX_FILES][MAX_FILENAME_SIZE], int *fileCount);
void handleFileUpload(int clientSocket, const char *userName, const char *fileName, size_t fileSize);
void processFileManagement(int clientSocket, const char *userName);
void handleClient(int clientSocket);

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

void createUser(int clientSocket)
{
    char userName[256];
    ssize_t userNameSize = recv(clientSocket, userName, sizeof(userName) - 1, 0);
    if (userNameSize <= 0)
    {
        perror("Error receiving username");
        close(clientSocket);
        return;
    }
    userName[userNameSize] = '\0';

    char filePath[300];
    snprintf(filePath, sizeof(filePath), "%s.config", userName);

    int fileDescriptor = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fileDescriptor < 0)
    {
        perror("Error creating config file");
        close(clientSocket);
        return;
    }

    send(clientSocket, "User created", strlen("User created"), 0);

    char password[MAX_SIZE];
    ssize_t passwordSize = recv(clientSocket, password, sizeof(password) - 1, 0);
    if (passwordSize <= 0)
    {
        perror("Error receiving password");
        close(fileDescriptor);
        close(clientSocket);
        return;
    }
    password[passwordSize] = '\0';

    dprintf(fileDescriptor, "%s\n*\n", password);
    printf("User registered with name: %s\n", userName);

    close(fileDescriptor);
}

int parseFileAfterAsterisk(const char *userName, char fileNames[MAX_FILES][MAX_FILENAME_SIZE], int *fileCount)
{
    char filePath[300];
    snprintf(filePath, sizeof(filePath), "%s.config", userName);

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

void handleFileUpload(int clientSocket, const char *userName, const char *fileName, size_t fileSize)
{
    char fileNames[MAX_FILES][MAX_FILENAME_SIZE]; // Array to store file names
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
     char filePath[300];
    snprintf(filePath, sizeof(filePath), "%s.config", userName);

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
        const char *successMsg = "File metadata updated.";
        send(clientSocket, successMsg, strlen(successMsg), 0);
        printf("File metadata updated\n");
    }
    

    close(fileDescriptor);
}

void authenticateUser(int clientSocket)
{
    char userName[256];
    ssize_t userNameSize = recv(clientSocket, userName, sizeof(userName) - 1, 0);
    if (userNameSize <= 0)
    {
        perror("Error receiving username");
        close(clientSocket);
        return;
    }
    userName[userNameSize] = '\0';

    char filePath[300];
    snprintf(filePath, sizeof(filePath), "%s.config", userName);

    if (access(filePath, F_OK) != 0)
    {
        send(clientSocket, "No file found, no user registered", strlen("No file found, no user registered"), 0);
        return;
    }

    int fileDescriptor = open(filePath, O_RDONLY);
    if (fileDescriptor < 0)
    {
        perror("Error opening config file");
        close(clientSocket);
        return;
    }

    char storedPassword[MAX_SIZE];
    ssize_t bytesRead = read(fileDescriptor, storedPassword, sizeof(storedPassword) - 1);
    if (bytesRead <= 0)
    {
        perror("Error reading password");
        close(fileDescriptor);
        close(clientSocket);
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
        send(clientSocket, "User found", strlen("User found"), 0);
        printf("User %s authenticated\n", userName); 
        processFileManagement(clientSocket, userName);
    }
    else
    {
        send(clientSocket, "Incorrect password", strlen("Incorrect password"), 0);
        printf("Incorrect password for user %s\n", userName);
    }
}

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
 
        handleFileUpload(clientSocket, userName, fileName, fileSize);

        //send(clientSocket, "File metadata updated", strlen("File metadata updated"), 0);
    }
    else if (option == 2)
    { 
        send(clientSocket, "File download functionality not implemented", strlen("File download functionality not implemented"), 0);
    }
}

void handleClient(int clientSocket)
{
    int option;
    ssize_t bytesReceived;
 
    bytesReceived = recv(clientSocket, &option, sizeof(option), 0);
    if (bytesReceived <= 0)
    {
        perror("Error receiving option");
        close(clientSocket);
        return;
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
}

int main()
{
    int serverSocket, clientSocket;
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
 
    while ((clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen)) >= 0)
    {
        printf("Client connected\n");
        handleClient(clientSocket);
    }

    if (clientSocket < 0)
    {
        perror("Error accepting connection");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    close(serverSocket);
    return 0;
}
