#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_SIZE 1024

// Define DT_REG if it's not already defined
#ifndef DT_REG
#define DT_REG 8
#endif

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
    char operation[2];
    while(1){
        printf("Server is listening again\n");
        ssize_t opSize = recv(clientSocket, operation, sizeof(operation) - 1, 0);
        if (opSize <= 0)
        {
            perror("Error receiving operation type");
            close(clientSocket);
            return;
        }
        operation[opSize] = '\0';

        if (strcmp(operation, "1") == 0) // Download file
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

            // Check if the file exists
            int fileDescriptor = open(fileName, O_RDONLY);
            if (fileDescriptor < 0)
            {
                send(clientSocket, "No file found", 13, 0);
                perror("File not found");
                close(clientSocket);
                return;
            }

            send(clientSocket, "$READY$", 7, 0); // Send readiness to client

            // Send file data
            char buffer[MAX_SIZE]={'\0'};
            ssize_t bytesRead;
            while ((bytesRead = read(fileDescriptor, buffer, sizeof(buffer))) > 0)
            {
                send(clientSocket, buffer, bytesRead, 0);
            }
            send(clientSocket,"END",3,0);
            printf("File sent successfully: %s\n", fileName);
            close(fileDescriptor);
        }
        else if (strcmp(operation, "2") == 0) // Upload file
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

            int fileDescriptor = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fileDescriptor < 0)
            {
                perror("Error creating file");
                close(clientSocket);
                return;
            }

            send(clientSocket, "$READY$", 7, 0);

            char buffer[MAX_SIZE];
            ssize_t bytesRead;
            while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
            {
                if(strncmp(buffer, "END", 3) == 0) break;
                write(fileDescriptor, buffer, bytesRead);
            }
            printf("Came Out");
            if (bytesRead < 0)
            {
                perror("Error receiving file data");
            }
            else
            {
                printf("File received and saved successfully.\n");
                send(clientSocket, "$SUCCESS$", 9, 0);
            }

            close(fileDescriptor);
        }
        else if (strcmp(operation, "3") == 0) // View files
        {
            listFiles(clientSocket);
        }
        else
        {
            printf("Invalid operation received\n");
            break;
        }
    }
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

   

    while (1)
    {
        printf("Server is listening on port 12345...\n");
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




















// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <string.h>
// #include <arpa/inet.h>
// #include <sys/socket.h>
// #include <fcntl.h>
// #include <sys/stat.h>

// #define MAX_SIZE 1024
// #define STORAGE_LIMIT 10240
// int checkStorageSpace()
// {
//     return STORAGE_LIMIT;
// }
// void HandleUpload(int clientSocket)
// {
//     char buffer[MAX_SIZE];
//     char command[256];
//     int recvSize = recv(clientSocket, command, sizeof(command) - 1, 0);
//     if (recvSize <= 0)
//     {
//         close(clientSocket);
//         return;
//     }
//     command[recvSize] = '\0';

//     if (strncmp(command, "$UPLOAD$", 8) != 0)
//     {
//         printf("Invalid command.\n");
//         close(clientSocket);
//         return;
//     }
    
//     char filePath[256];
//     strcpy(filePath, command +8);
//     filePath[sizeof(filePath)-1]='\0';
//     int availableSpace = checkStorageSpace();
//     if (availableSpace <= 0)
//     {
//         send(clientSocket, "$FAILURE$LOW_SPACE$", 20, 0);
//         close(clientSocket);
//         return;
//     }

//     send(clientSocket, "$SUCCESS$", 9, 0);


//     int fileDescriptor = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
//     if (fileDescriptor < 0)
//     {
//         perror("Error creating file");
//         close(clientSocket);
//         return;
//     }

//     ssize_t bytesRead;
//     while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
//     {
//         if(!strncmp(buffer,"END",3))
//             write(fileDescriptor, buffer, bytesRead);
//         else break;
//     }

//     if (bytesRead < 0)
//     {
//         perror("Error receiving file data");
//     }
   
//     send(clientSocket, "$SUCCESS$", 9, 0);
//     printf("Succesfully Received and saved: %s",filePath);
//     close(fileDescriptor);
//     close(clientSocket);
// }
// void  HandleDownload(int clientSocket)
// {
    
//     char command[256];
//     int recvSize = recv(clientSocket, command, sizeof(command) - 1, 0);
//     if (recvSize <= 0)
//     {
//         close(clientSocket);
//         return;
//     }
//     command[recvSize-1] = '\0';
//     if(strncmp(command,"$DOWNLOAD$",10)){printf("Invalid Command!!!\n");return;}
//     char filePath[256];
//     strcpy(filePath,command + 10);
//     int fileDescriptor = open(filePath, O_RDONLY);
//     if (fileDescriptor < 0)
//     {
//         perror("Error opening file");
//         send(clientSocket,"$FILENAME_INCORRECT$",20,0);
//         return;
//     }
//     send(clientSocket,"$READY$",6,0);
   
//     char buffer[MAX_SIZE];
//     ssize_t bytesRead;
//     while ((bytesRead = read(fileDescriptor, buffer, sizeof(buffer))) > 0)
//     {
//         send(clientSocket, buffer, bytesRead, 0);
//     }
//     if(send(clientSocket,"END",3,0)<0)
//     {
//         printf("Error in Sending Data!!!!\n");
//     }
//     close(fileDescriptor);
//     printf("File data sent successfully.\n");
//     char response[256];
//     int bytes = recv(clientSocket, response, sizeof(response) - 1, 0);
//     response[bytes - 1] = '\0';
//     printf("Client response: %s\n", response);  
//     close(clientSocket);
// }
// void handleClient(int clientSocket)
// {
    
//     char val[1];
//     int numrcv = recv(clientSocket,val,sizeof(val),0);
//     if(val[0]==1)
//     {
//         HandleUpload(clientSocket);
//     }
//     else{
//         HandleDownload(clientSocket);
//     }
   
// }

// int main()
// {
//     int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
//     if (serverSocket < 0)
//     {
//         perror("Failed to create socket");
//         return 1;
//     }

//     struct sockaddr_in serverAddr;
//     serverAddr.sin_family = AF_INET;
//     serverAddr.sin_addr.s_addr = INADDR_ANY;
//     serverAddr.sin_port = htons(12345);

//     if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
//     {
//         perror("Bind failed");
//         close(serverSocket);
//         return 1;
//     }

//     if (listen(serverSocket, SOMAXCONN) < 0)
//     {
//         perror("Listen failed");
//         close(serverSocket);
//         return 1;
//     }

//     printf("Server is listening on port 12345...\n");

//     while (1)
//     {
//         struct sockaddr_in clientAddr;
//         socklen_t clientAddrLen = sizeof(clientAddr);
//         int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrLen);
//         if (clientSocket < 0)
//         {
//             perror("Accept failed");
//             continue;
//         }

//         handleClient(clientSocket);
//     }

//     close(serverSocket);
//     return 0;
// }
