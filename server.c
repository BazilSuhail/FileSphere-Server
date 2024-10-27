#include "helper.h"
#define PORT 8000

UserInfo *users[MAX_CONNECTIONS] = {NULL};
int currentConnections = 0;
pthread_mutex_t globalMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t connectionCond = PTHREAD_COND_INITIALIZER;


// Helper function to get or initialize UserInfo
UserInfo *getUserInfo(const char *userName)
{
    pthread_mutex_lock(&globalMutex);
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (users[i] && strcmp(users[i]->userName, userName) == 0)
        {
            pthread_mutex_unlock(&globalMutex);
            return users[i];
        }
    }
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        if (users[i] == NULL)
        {
            users[i] = (UserInfo *)malloc(sizeof(UserInfo));
            strncpy(users[i]->userName, userName, sizeof(users[i]->userName));
            users[i]->readCount = 0;
            users[i]->isWriting = 0;
            users[i]->queueFront = users[i]->queueBack = 0;
            pthread_mutex_init(&users[i]->mutex, NULL);
            pthread_cond_init(&users[i]->queueCond, NULL);
            pthread_mutex_unlock(&globalMutex);
            return users[i];
        }
    }
    pthread_mutex_unlock(&globalMutex);
    return NULL;
}

void enqueueRequest(UserInfo *user, Request *request)
{
    user->requestQueue[user->queueBack] = request;
    user->queueBack = (user->queueBack + 1) % MAX_CONNECTIONS;
}

Request *dequeueRequest(UserInfo *user)
{
    if (user->queueFront == user->queueBack)
        return NULL;
    Request *request = user->requestQueue[user->queueFront];
    user->queueFront = (user->queueFront + 1) % MAX_CONNECTIONS;
    return request;
}

void processQueue(UserInfo *user)
{
    if (user->queueFront == user->queueBack)
        return;

    Request *nextRequest = user->requestQueue[user->queueFront];
    if (nextRequest->type == READ && !user->isWriting)
    {
        pthread_cond_broadcast(&nextRequest->cond);
    }
    else if (nextRequest->type == WRITE && user->readCount == 0 && !user->isWriting)
    {
        pthread_cond_signal(&nextRequest->cond);
    }
}

void *handleClient(void *clientSocketPtr)
{
    int clientSocket = *((int *)clientSocketPtr);
    free(clientSocketPtr);

    pthread_mutex_lock(&globalMutex);
    if (currentConnections >= MAX_CONNECTIONS)
    {
        const char *messageBusy = "Server is busy. Please try again later\0";
        send(clientSocket, messageBusy, strlen(messageBusy) + 1, 0);
        close(clientSocket);
        pthread_mutex_unlock(&globalMutex);
        return NULL;
    }
    const char *messageReady = "Server is ready.\0";
    send(clientSocket, messageReady, strlen(messageReady) + 1, 0);
    currentConnections++;
    pthread_mutex_unlock(&globalMutex);

    // =====================================================================
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
    // ============================================================== 
    close(clientSocket);
    // Lock mutex to safely update the connection counter and signal waiting threads
    pthread_mutex_lock(&globalMutex);
    currentConnections--;
    // Signal one of the waiting threads that a connection slot is available
    pthread_cond_signal(&connectionCond);
    pthread_mutex_unlock(&globalMutex);
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

    printf("Server listening on port %d...\n", PORT);

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
