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
        printf("Failed to open directory");
        send(clientSocket, "Error opening directory", 24, 0);
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            stat(entry->d_name, &fileStat);
            snprintf(fileInfo, sizeof(fileInfo), "File: %s, Size: %ld bytes, Created: %ld\n",
                     entry->d_name, fileStat.st_size, fileStat.st_ctime);
            send(clientSocket, fileInfo, strlen(fileInfo), 0);
        }
    }

    closedir(dir);
    send(clientSocket, "$END$", 5, 0);
}
void storeUser(const char *username, const char *password) {
    FILE *file = fopen("users.txt", "a"); // Open file in append mode
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    
    // Write the username and password to the file in "username:password" format
    fprintf(file, "%s:%s\n", username, password);
    
    fclose(file);
}

// Function to validate username and password
int validateUser(const char *username, const char *password) {
    char line[256]; 
    char storedUsername[50], storedPassword[50];

    FILE *file = fopen("users.txt", "r"); // Open file in read mode
    if (file == NULL) {
        perror("Error opening file");
        return 0;
    }
    
    // Read each line of the file and split by ':'
    while (fgets(line, sizeof(line), file)) {
        // Split the line into username and password
        char *token = strtok(line, ":");
        if (token != NULL) {
            strcpy(storedUsername, token); // First token is the username
            
            token = strtok(NULL, "\n");
            if (token != NULL) {
                strcpy(storedPassword, token); // Second token is the password
            }
            
            // Compare the input username and password with the stored ones
            if (strcmp(username, storedUsername) == 0 && strcmp(password, storedPassword) == 0) {
                fclose(file);
                return 1; // User exists
            }
        }
    }
    
    fclose(file);
    return 0; // User does not exist
}

void Authentication(int clientSocket){
     char operation[2];
    ssize_t opSize = recv(clientSocket, operation, sizeof(operation) - 1, 0);   
    char username[50];
    int bytes = recv(clientSocket, username, sizeof(username) - 1, 0);
    if (bytes <= 0)
    {
        printf("Error receiving file name");
        close(clientSocket);
        return;
    }
    username[bytes] = '\0';
    char Password[50];
    bytes = recv(clientSocket, Password, sizeof(username) - 1, 0);
    if (bytes <= 0)
    {
        printf("Error receiving file name");
        close(clientSocket);
        return;
    }
    Password[bytes] = '\0';
    if (strcmp(operation, "1") == 0)  // Login
    {
        if(validateUser(username,Password)==0){
            send(clientSocket,"3",1,0);
            printf("Login Failed");
            return;
        }else{
            send(clientSocket,"1",1,0); 
            printf("Succesfully Login %s",username);
        }
    }
    else if (strcmp(operation, "2") == 0)  //SIGNUP
    {
        storeUser(username,Password);
        send(clientSocket,"2",1,0);
        printf("Succesfully Login");
    }
}
void handleClient(int clientSocket)
{
    Authentication(clientSocket);
    char operation[2];
    ssize_t opSize = recv(clientSocket, operation, sizeof(operation) - 1, 0); 
    if (opSize <= 0)
    {
        printf("Error receiving operation type");
        close(clientSocket);
        return;
    }
    operation[opSize] = '\0';

    if (strcmp(operation, "1") == 0)  //Download Handle
    {
        char fileName[256];
        ssize_t fileNameSize = recv(clientSocket, fileName, sizeof(fileName) - 1, 0);
        if (fileNameSize <= 0)
        {
            printf("Error receiving file name");
            close(clientSocket);
            return;
        }
        fileName[fileNameSize] = '\0';
 
 
        int fileDescriptor = open(fileName, O_RDONLY);
        if (fileDescriptor < 0)
        {
            send(clientSocket, "No file found", 13, 0);
            printf("File not found");
            close(clientSocket);
            return;
        }

        send(clientSocket, "$READY$", 7, 0); 
 
        char buffer[MAX_SIZE];
        ssize_t bytesRead;
        while ((bytesRead = read(fileDescriptor, buffer, sizeof(buffer))) > 0)
        {
            send(clientSocket, buffer, bytesRead, 0);
        }

        printf("File sent successfully: %s\n", fileName);
        close(fileDescriptor);
    }
    else if (strcmp(operation, "2") == 0)
    {
        char fileName[256];
        
        ssize_t fileNameSize = recv(clientSocket, fileName, sizeof(fileName) - 1, 0);
        if (fileNameSize <= 0)
        {
            printf("Error receiving file name");
            close(clientSocket);
            return;
        }
        fileName[fileNameSize] = '\0';

        printf("Receiving file: %s\n", fileName);
 
        int fileDescriptor = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fileDescriptor < 0)
        {
            printf("Error creating file");
            close(clientSocket);
            return;
        }
 
        send(clientSocket, "$READY$", 7, 0);
 
        char buffer[MAX_SIZE];
        ssize_t bytesRead;

        while (1)
        { 
            bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead < 0)
            {
                printf("Error receiving file data");
                close(fileDescriptor);
                close(clientSocket);
                return;
            }
            if (bytesRead == 6 && strncmp(buffer, "$DONE$", 6) == 0)
            { 
                printf("File transfer completed.\n");
                break;
            }
            if(strncmp(buffer,"$END$",5)==1){
            if (write(fileDescriptor, buffer, bytesRead) < 0)
            {
                printf("Error writing to file");
                close(fileDescriptor);
                close(clientSocket);
                return;
            }
            }
        }
 
        send(clientSocket, "$SUCCESS$", 9, 0);
 
        close(fileDescriptor);
    }
    else if (strcmp(operation, "3") == 0)
    {
        listFiles(clientSocket);
    }
    else
    {
        printf("Invalid operation received\n");
    }

    close(clientSocket);
}

int main()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        printf("Failed to create socket");
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(12345);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("Bind failed");
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) < 0)
    {
        printf("Listen failed");
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
            printf("Accept failed");
            continue;
        }

        handleClient(clientSocket);
    }

    close(serverSocket);
    return 0;
}
