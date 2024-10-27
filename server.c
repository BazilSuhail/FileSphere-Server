#include "helper.h"
#define PORT 8000

UserInfo *users[MAX_CONNECTIONS] = {NULL}; // Define users with NULL initialization if needed
int currentConnections = 0;
pthread_mutex_t globalMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t connectionCond = PTHREAD_COND_INITIALIZER;

// Helper function to get or initialize UserInfo
/*UserInfo *getUserInfo(const char *userName) {
    pthread_mutex_lock(&globalMutex);
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (users[i] && strcmp(users[i]->userName, userName) == 0) {
            pthread_mutex_unlock(&globalMutex);
            return users[i];
        }
    }
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (users[i] == NULL) {
            users[i] = (UserInfo *)malloc(sizeof(UserInfo));
            strncpy(users[i]->userName, userName, sizeof(users[i]->userName));
            users[i]->readCount = 0;
            users[i]->isWriting = 0;
            users[i]->connectionCount = 0; // Initialize connection count
            pthread_mutex_init(&users[i]->mutex, NULL);
            pthread_cond_init(&users[i]->cond, NULL);
            pthread_mutex_unlock(&globalMutex);
            return users[i];
        }
    }
    pthread_mutex_unlock(&globalMutex);
    return NULL;
}
*/
// Helper function to get or initialize UserInfo
UserInfo *getUserInfo(const char *userName) {
    pthread_mutex_lock(&globalMutex);
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (users[i] && strcmp(users[i]->userName, userName) == 0) {
            pthread_mutex_unlock(&globalMutex);
            return users[i];
        }
    }
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (users[i] == NULL) {
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

// Enqueue a request to a user's queue
void enqueueRequest(UserInfo *user, Request *request) {
    user->requestQueue[user->queueBack] = request;
    user->queueBack = (user->queueBack + 1) % MAX_CONNECTIONS;
}

// Dequeue a request from a user's queue
Request *dequeueRequest(UserInfo *user) {
    if (user->queueFront == user->queueBack) return NULL;
    Request *request = user->requestQueue[user->queueFront];
    user->queueFront = (user->queueFront + 1) % MAX_CONNECTIONS;
    return request;
}

// Process the next request in the user's queue
void processQueue(UserInfo *user) {
    if (user->queueFront == user->queueBack) return;

    Request *nextRequest = user->requestQueue[user->queueFront];
    if (nextRequest->type == READ && !user->isWriting) {
        pthread_cond_broadcast(&nextRequest->cond);
    } else if (nextRequest->type == WRITE && user->readCount == 0 && !user->isWriting) {
        pthread_cond_signal(&nextRequest->cond);
    }
}

// Client handler function

void *handle_auth(int clientSocket) {
    //int clientSocket = *(int *)arg;
    //free(arg);

    pthread_mutex_lock(&globalMutex);
    while (currentConnections >= MAX_CONNECTIONS) {
        pthread_cond_wait(&connectionCond, &globalMutex);
    }
    currentConnections++;
    pthread_mutex_unlock(&globalMutex);

    authenticateUser(clientSocket);

    close(clientSocket);

    pthread_mutex_lock(&globalMutex);
    currentConnections--;
    pthread_cond_signal(&connectionCond);
    pthread_mutex_unlock(&globalMutex);

    return NULL;
}

/*
void *handleClient(void *clientSocketPtr)
{
    int clientSocket = *((int *)clientSocketPtr);
    free(clientSocketPtr);
    
    // Lock the mutex to safely check the currentConnections
    pthread_mutex_lock(&globalMutex);

    if (currentConnections >= MAX_CONNECTIONS)
    {
        // Notify the client that the server is busy
        const char *busyMessage = "Server is busy. Please try again later.";
        send(clientSocket, busyMessage, strlen(busyMessage) + 1, 0);
        // Close the socket and release the lock
        close(clientSocket);
        pthread_mutex_unlock(&globalMutex);
        return NULL;
    }

    // Increase connection count as we're about to handle a client
    manageConnections(1);
    pthread_mutex_unlock(&globalMutex); 
    int option;
    ssize_t bytesReceived;
    bytesReceived = recv(clientSocket, &option, sizeof(option), 0);
    printf("opt: %d", option);
    if (bytesReceived <= 0)
    {
        perror("Error receiving option");
        close(clientSocket);
        manageConnections(-1);
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

    // Decrement connection count after finishing handling the client
    manageConnections(-1);
    return NULL;
}*/

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
        handle_auth(clientSocket);
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
        printf("Client Disconnected\n");

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
