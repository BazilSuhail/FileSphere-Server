#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define MAX_SIZE 1024

// Function declarations
void createUser(int clientSocket);
void authenticateUser(int clientSocket);
void handleClient(int clientSocket);

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

    // Create the file path based on the username
    char filePath[300];
    snprintf(filePath, sizeof(filePath), "%s.config", userName);

    // Check if the file exists
    if (access(filePath, F_OK) == 0)
    {
        send(clientSocket, "File exists, cannot create user", strlen("File exists, cannot create user"), 0);
        return;
    }

    // Create the file for the user
    int fileDescriptor = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fileDescriptor < 0)
    {
        perror("Error creating config file");
        close(clientSocket);
        return;
    }

    // Send message to client indicating that user was created
    send(clientSocket, "User created", strlen("User created"), 0);

    // Receive password from client
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

    // Write password to the config file with "\n*\n" at the end
    dprintf(fileDescriptor, "%s\n*\n", password);
    printf("User registered with name: %s\n", userName);

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

    // Check if the file exists
    if (access(filePath, F_OK) != 0)
    {
        send(clientSocket, "No file found, no user registered", strlen("No file found, no user registered"), 0);
        return;
    }

    // Send message indicating the user was found
    send(clientSocket, "User found", strlen("User found"), 0);

    // Read stored password from the config file
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

    // Find the password up to the "*"
    char *passwordEnd = strchr(storedPassword, '*');
    if (passwordEnd != NULL)
    {
        *passwordEnd = '\0'; // Terminate password at '*'
    }
    // Remove trailing newline if present
    size_t len = strlen(storedPassword);
    if (len > 0 && storedPassword[len - 1] == '\n')
    {
        storedPassword[len - 1] = '\0'; // Remove the newline
    }
    close(fileDescriptor);

    // Receive password from client for authentication
    char inputPassword[MAX_SIZE];
    ssize_t passwordSize = recv(clientSocket, inputPassword, sizeof(inputPassword) - 1, 0);
    if (passwordSize <= 0)
    {
        perror("Error receiving password");
        close(clientSocket);
        return;
    }
    inputPassword[passwordSize] = '\0';

    // Compare the passwords
    if (strcmp(storedPassword, inputPassword) == 0)
    {
        send(clientSocket, "User Authenticated, you are logged in", strlen("User Authenticated, you are logged in"), 0);
        printf("User %s authenticated\n", userName);
    }
    else
    {
        send(clientSocket, "Incorrect password", strlen("Incorrect password"), 0);
        printf("Incorrect password for user %s\n", userName);
    }
}

void handleClient(int clientSocket)
{
    int option;
    recv(clientSocket, &option, sizeof(option), 0);

    if (option == 1)
    {
        createUser(clientSocket);
    }
    else if (option == 2)
    {
        authenticateUser(clientSocket);
    }
}

int main()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        perror("Error creating socket");
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("Error binding socket");
        return 1;
    }

    if (listen(serverSocket, 5) == -1)
    {
        perror("Error listening on socket");
        return 1;
    }

    printf("Server is listening on port 8082...\n");

    while (1)
    {
        int clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == -1)
        {
            perror("Error accepting client connection");
            continue;
        }

        handleClient(clientSocket);
        close(clientSocket);
    }

    close(serverSocket);
    return 0;
}
