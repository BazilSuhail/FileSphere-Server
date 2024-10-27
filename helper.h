// my_functions.h
#ifndef MY_FUNCTIONS_H
#define MY_FUNCTIONS_H

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
#include <stddef.h>

#define MAX_SIZE 1024
#define MAX_STORAGE 50 * 1024

#define MAX_FILES 100
#define MAX_FILENAME_SIZE 100

#define MAX_CONNECTIONS 2

typedef enum
{
    READ,
    WRITE
} RequestType;

typedef struct
{
    RequestType type;
    pthread_cond_t cond;
} Request;

// Define user structure with synchronization components
/*typedef struct {
    char userName[256];
    int readCount;
    int isWriting;
    int connectionCount; // Count of active connections for this user
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} UserInfo;
*/

typedef struct
{
    char userName[256];
    int readCount;
    int isWriting;
    pthread_mutex_t mutex;
    int connectionCount;
    pthread_cond_t queueCond;
    Request *requestQueue[MAX_CONNECTIONS];
    int queueFront, queueBack;
} UserInfo;

extern UserInfo *users[MAX_CONNECTIONS];
extern int currentConnections;
extern pthread_mutex_t globalMutex;
extern pthread_cond_t connectionCond;

// extern pthread_mutex_t mutex;
// extern pthread_cond_t readers_cond;
// extern pthread_cond_t writers_cond;

// extern int readers_count;
// extern int writers_waiting;
// extern int writer_active;

// queue structure
void enqueueRequest(UserInfo *user, Request *request);
Request *dequeueRequest(UserInfo *user);
void processQueue(UserInfo *user);

//  user info funciton
UserInfo *getUserInfo(const char *userName);

// authenticate.c funcitons
void process_task_managment(int clientSocket, const char *userName);
void createUser(int clientSocket);
void authenticateUser(int clientSocket);

// queries.c funcitons
int viewFile(int clientSocket, const char *userName);
void sendFileToClient(int clientSocket, const char *userName);
void receiveFileFromClient(int clientSocket, const char *userName);
void delete_File_from_user_Config(int clientSocket, const char *userName, const char *fileName);
void receive_updated_file_content(int clientSocket, const char *userName);

// parsingData.c funcitons
int calculateSumOfSizes(int *sizes, int count);
int checkFileExists(char fileNames[MAX_FILES][MAX_FILENAME_SIZE], int fileCount, const char *inputFileName);

void receive_replace_able_file_content(int clientSocket, const char *userName, const char *fileNameParam);
void updateFileCount(int clientSocket, const char *userName, const char *targetFile);
void handleFileExists(int clientSocket, const char *fileName, const char *userName, const char *ActualFile);

int parseFileAfterAsterisk(const char *userName, char fileNames[MAX_FILES][MAX_FILENAME_SIZE], int *fileCount);
void write_FileInfo_to_user_Config(int clientSocket, const char *userName, const char *fileName, size_t fileSize);

// sync
void startRead(UserInfo *user);
void finishRead(UserInfo *user);

void startWrite(UserInfo *user);
void finishWrite(UserInfo *user);

#endif
