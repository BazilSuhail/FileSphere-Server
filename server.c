#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#ifndef DT_REG
#define DT_REG 8
#endif

#define MAX_SIZE 1024
#define STORAGE_LIMIT 10240

int checkStorageSpace()
{
    return STORAGE_LIMIT;
}
void HandleUpload(int clientSocket)
{
    char buffer[MAX_SIZE];
    char command[256];
    int recvSize = recv(clientSocket, command, sizeof(command) - 1, 0);
    if (recvSize <= 0)
    {
        close(clientSocket);
        return;
    }
    command[recvSize] = '\0';

    if (strncmp(command, "$UPLOAD$", 8) != 0)
    {
        printf("Invalid command.\n");
        close(clientSocket);
        return;
    }

    char filePath[256];
    strcpy(filePath, "test.txt");

    int availableSpace = checkStorageSpace();
    if (availableSpace <= 0)
    {
        send(clientSocket, "$FAILURE$LOW_SPACE$", 20, 0);
        close(clientSocket);
        return;
    }

    send(clientSocket, "$SUCCESS$", 9, 0);


    int fileDescriptor = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fileDescriptor < 0)
    {
        perror("Error creating file");
        close(clientSocket);
        return;
    }

    ssize_t bytesRead;
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
    {
        write(fileDescriptor, buffer, bytesRead);
    }

    if (bytesRead < 0)
    {
        perror("Error receiving file data");
    }
    else
    {
        send(clientSocket, "$SUCCESS$", 9, 0);
    }

    close(fileDescriptor);
    close(clientSocket);
}
void  HandleDownload(int clientSocket)
{
    char buffer[MAX_SIZE];
    char command[256];
    int recvSize = recv(clientSocket, command, sizeof(command) - 1, 0);
    if (recvSize <= 0)
    {
        close(clientSocket);
        return;
    }
    
    char filePath[256];
    strcpy(filePath, "test.txt");
    
    int fileDescriptor = open(filePath, O_RDONLY);
    if (fileDescriptor < 0)
    {
        perror("Error opening file");
        return;
    }

    char command[256];
    snprintf(command, sizeof(command), "$UPLOAD$%s$", filePath);
    send(clientSocket, command, strlen(command), 0);

    char response[256];
    recv(clientSocket, response, sizeof(response) - 1, 0);
    response[sizeof(response) - 1] = '\0';
    if (strcmp(response, "$SUCCESS$") == 0)
    {
        printf("Server response: %s\n", response);
        close(fileDescriptor);
        return;
    }


    char buffer[MAX_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = read(fileDescriptor, buffer, sizeof(buffer))) > 0)
    {
        send(clientSocket, buffer, bytesRead, 0);
    }

 
    close(fileDescriptor);
    printf("File data sent successfully.\n");

    recv(clientSocket, response, sizeof(response) - 1, 0);
    response[sizeof(response) - 1] = '\0';
    printf("Server response: %s\n", response);
}
void listFiles(int clientSocket)
{
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;
    char fileInfo[MAX_SIZE];

    dir = opendir(".");
    if (dir == NULL)
    {
        perror("Failed to open directory");
        send(clientSocket, "Error opening directory", 24, 0);
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG) // Regular file
        {
            stat(entry->d_name, &fileStat);
            snprintf(fileInfo, sizeof(fileInfo), "File: %s, Size: %ld bytes, Created: %ld\n",
                     entry->d_name, fileStat.st_size, fileStat.st_ctime);
            send(clientSocket, fileInfo, strlen(fileInfo), 0);
        }
    }

    closedir(dir);
    send(clientSocket, "$END$", 5, 0); // End of file list
}

void handleClient(int clientSocket)
{
    
    char val[1];
    int numrcv = recv(clientSocket,val,sizeof(val),0);
    if(val[0]==1)
    {
        HandleUpload(clientSocket);
    }
    else if(val[0]==2)
    {
        HandleDownload(clientSocket);
    }
    else{
        listFiles(clientSocket);
    }
   
}

int main()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        perror("Failed to create socket");
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(12345);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Bind failed");
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) < 0)
    {
        perror("Listen failed");
        close(serverSocket);
        return 1;
    }

    printf("Server is listening on port 12345...\n");

    while (1)
    {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (clientSocket < 0)
        {
            perror("Accept failed");
            continue;
        }

        handleClient(clientSocket);
    }

    close(serverSocket);
    return 0;
}
