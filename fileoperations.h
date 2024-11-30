#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#define MAX_FILES 100
#define MAX_FILENAME_SIZE 100
typedef struct
{
    char operation;
    char *filename;
    size_t fileSize;
    char *userName;
    int clientSocket;
} FileOperation;

FileOperation* createFileOperation()
{
    FileOperation *operation = (FileOperation *)malloc(sizeof(FileOperation));
    if (operation == NULL)
    {
        perror("Failed to allocate memory for FileOperation");
        exit(EXIT_FAILURE);
    }

    operation->filename = (char *)malloc(MAX_FILENAME_SIZE * sizeof(char));
    operation->userName = (char *)malloc(256 * sizeof(char));

    if (operation->filename == NULL || operation->userName == NULL)
    {
        perror("Failed to allocate memory for FileOperation fields");
        free(operation->filename);
        free(operation->userName);
        free(operation);
        exit(EXIT_FAILURE);
    }

    operation->operation = '\0';
    operation->fileSize = 0;
    operation->clientSocket = -1;

    return operation;
}

void destroyFileOperation(FileOperation *operation)
{
    if (operation)
    {
        free(operation->filename);
        free(operation->userName);
        free(operation);
    }
}
