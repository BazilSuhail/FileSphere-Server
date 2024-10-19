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

// authenticate.c funcitons
void processFileManagement(int clientSocket, const char *userName);
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

 
#endif
