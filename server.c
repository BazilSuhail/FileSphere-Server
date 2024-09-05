#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_SIZE 1024
#define STORAGE_LIMIT 10240
int checkStorageSpace()
{
    return STORAGE_LIMIT;
}

void handleClient(int clientSocket)
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
