#include "helper.h"

/* =====================================================================
                        View Files Functionality
========================================================================  */
int viewFile(int clientSocket, const char *userName)
{
    char user_config[1024];
    // snprintf(user_config, sizeof(user_config), "%s.config", userName);
    snprintf(user_config, sizeof(user_config), "%s/%s.config", userName, userName);

    int fileDescriptor = open(user_config, O_RDONLY);
    if (fileDescriptor < 0)
    {
        perror("Error opening config file");
        return -1;
    }

    char view_buffer[MAX_SIZE];
    ssize_t bytesReads;
    int foundAsterisk = 0;

    while ((bytesReads = read(fileDescriptor, view_buffer, sizeof(view_buffer) - 1)) > 0)
    {
        view_buffer[bytesReads] = '\0';

        if (!foundAsterisk)
        {
            char *asteriskPos = strchr(view_buffer, '*');
            if (asteriskPos != NULL)
            {
                foundAsterisk = 1;
                char *newlinePos = strchr(asteriskPos, '\n');
                if (newlinePos != NULL)
                {
                    newlinePos++;
                    memmove(view_buffer, newlinePos, bytesReads - (newlinePos - view_buffer));
                    bytesReads -= (newlinePos - view_buffer);
                    view_buffer[bytesReads] = '\0';
                }
                else
                {
                    continue;
                }
            }
        }

        if (foundAsterisk)
        {
            send(clientSocket, view_buffer, bytesReads, 0);
            printf("%s", view_buffer);
        }
    }

    if (bytesReads < 0)
    {
        perror("Error reading config file");
        close(fileDescriptor);
        return -2;
    }

    printf("\nData about Files sent successfully \n");
    close(fileDescriptor);
    return 0;
}

/* =====================================================================
                Send and Recieve File to the Client and View Files
   =====================================================================  */
void sendFileToClient(int clientSocket, const char *userName)
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

    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s", userName, fileName);

    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        send(clientSocket, "No file found", 13, 0);
        perror("File not found");
        close(clientSocket);
        return;
    }

    send(clientSocket, "$READY$", 7, 0);

    char buffer[MAX_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        ssize_t sentBytes = send(clientSocket, buffer, bytesRead, 0);
        if (sentBytes < 0)
        {
            perror("Error sending file data");
            break;
        }
    }

    printf("File sent successfully: %s\n", filePath);
    fclose(file);
}

void receiveFileFromClient(int clientSocket, const char *userName)
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

    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s", userName, fileName);

    FILE *encoded_file = fopen(filePath, "w");
    if (encoded_file == NULL)
    {
        perror("Error creating file");
        close(clientSocket);
        return;
    }

    send(clientSocket, "$READY$", 7, 0);

    char buffer[1024];
    ssize_t bytesRead;
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
    {
        fwrite(buffer, sizeof(char), bytesRead, encoded_file);
    }

    if (bytesRead < 0)
    {
        perror("Error receiving file data");
    }
    else
    {
        printf("File received and saved successfully in %s.\n", filePath);
        send(clientSocket, "$SUCCESS$", 9, 0);
    }

    fclose(encoded_file);
}

/* =====================================================================
            Delete file req from user
========================================================================  */
void delete_File_from_user_Config(int clientSocket, const char *userName, const char *fileName)
{
    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s.config", userName, userName);

    char userFilePath[1024];
    snprintf(userFilePath, sizeof(userFilePath), "%s/%s", userName, fileName);

    if (access(userFilePath, F_OK) != 0)
    {
        const char *fileNotFoundMsg = "File not found.";
        send(clientSocket, fileNotFoundMsg, strlen(fileNotFoundMsg), 0);
        printf("File '%s' not found for user: %s\n", fileName, userName);
        return;
    }

    const char *fileFoundMsg = "File is found!!";
    send(clientSocket, fileFoundMsg, strlen(fileFoundMsg), 0);
    printf("File '%s' found for user: %s\n", fileName, userName);

    FILE *configFile = fopen(filePath, "r");
    if (!configFile)
    {
        perror("Error opening config file");
        const char *errorMsg = "Error opening config file.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    char password[256];
    char line[1024];
    char fileNames[MAX_FILES][MAX_FILENAME_SIZE];
    int fileSizes[MAX_FILES];
    int fileCount = 0;
    int fileIndex = -1;

    if (fgets(password, sizeof(password), configFile) == NULL)
    {
        perror("Error reading password from config file");
        fclose(configFile);
        const char *errorMsg = "Error reading config file.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    if (fgets(line, sizeof(line), configFile) == NULL || strcmp(line, "*\n") != 0)
    {
        perror("Error reading asterisk line from config file");
        fclose(configFile);
        const char *errorMsg = "Error reading config file.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    while (fgets(line, sizeof(line), configFile) != NULL)
    {
        char tempFileName[MAX_FILENAME_SIZE];
        int tempFileSize;

        if (sscanf(line, "%s - %d", tempFileName, &tempFileSize) == 2)
        {
            strcpy(fileNames[fileCount], tempFileName);
            fileSizes[fileCount] = tempFileSize;

            if (strcmp(tempFileName, fileName) == 0)
            {
                fileIndex = fileCount;
            }

            fileCount++;
        }
    }

    fclose(configFile);

    if (fileIndex == -1)
    {
        const char *fileNotFoundInConfig = "File not found in config.";
        send(clientSocket, fileNotFoundInConfig, strlen(fileNotFoundInConfig), 0);
        printf("File '%s' not found in config for user: %s\n", fileName, userName);
        return;
    }

    if (remove(userFilePath) != 0)
    {
        perror("Error deleting file from user's folder");
        const char *deleteError = "Error deleting file.";
        send(clientSocket, deleteError, strlen(deleteError), 0);
        return;
    }

    FILE *configFileWrite = fopen(filePath, "w");
    if (!configFileWrite)
    {
        perror("Error opening config file for writing");
        const char *errorMsg = "Error updating config file.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    fprintf(configFileWrite, "%s", password);
    fprintf(configFileWrite, "*\n");

    for (int i = 0; i < fileCount; i++)
    {
        if (i != fileIndex)
        {
            fprintf(configFileWrite, "%s - %d\n", fileNames[i], fileSizes[i]);
        }
    }

    fclose(configFileWrite);

    const char *successMsg = "File successfully deleted.";
    send(clientSocket, successMsg, strlen(successMsg), 0);
    printf("File '%s' successfully deleted from user: %s\n", fileName, userName);
}

/* =====================================================================
                 Update the file requested from user
========================================================================  */
void receive_updated_file_content(int clientSocket, const char *userName)
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
    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s", userName, fileName);

    FILE *encoded_file = fopen(filePath, "w+");
    if (encoded_file == NULL)
    {
        perror("Error creating or opening file");
        close(clientSocket);
        return;
    }
    send(clientSocket, "$READY$", 7, 0);
    char buffer[1024];
    ssize_t bytesRead;
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0)
    {
        fwrite(buffer, sizeof(char), bytesRead, encoded_file);
    }

    if (bytesRead < 0)
    {
        perror("Error receiving file data");
    }
    else
    {
        printf("File received and saved successfully in %s.\n", filePath);
        send(clientSocket, "$SUCCESS$", 9, 0);
    }
    fclose(encoded_file);
}

