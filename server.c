#include "helper.h" 
#define PORT 8000
 
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t readers_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t writers_cond = PTHREAD_COND_INITIALIZER;

int readers_count = 0;  // Number of active readers
int writers_waiting = 0; // Number of writers waiting
int writer_active = 0;   // Writer currently writing

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
    serverAddr.sin_port = htons(PORT);

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

    printf("Server listening on port %d...\n",PORT);

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
