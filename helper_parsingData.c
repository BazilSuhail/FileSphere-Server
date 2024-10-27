#include "helper.h"

int calculateSumOfSizes(int *sizes, int count)
{
    int sum = 0;
    for (int i = 0; i < count; i++)
    {
        sum += sizes[i];
    }
    return sum;
}

/* ======    User file storage checking Files Functionality     =====  */

void receive_replace_able_file_content(int clientSocket, const char *userName, const char *fileNameParam)
{
    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s", userName, fileNameParam);
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
        printf("File received and saved successfully as %s.\n", filePath);
        send(clientSocket, "$SUCCESS$", 9, 0);
    }

    // Close the file after writing
    fclose(encoded_file);
}

void updateFileCount(int clientSocket, const char *userName, const char *targetFile)
{
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "%s/%s_fileList.config", userName, userName);
    FILE *file = fopen(filePath, "r+");
    if (!file)
    {
        printf("Error: Could not open file %s\n", filePath);
        return;
    }

    char fileNames[MAX_FILES][MAX_FILENAME_SIZE];
    int fileCounts[MAX_FILES];
    int fileIndex = 0;

    char line[256];
    while (fgets(line, sizeof(line), file))
    { // Read each line
        char fileName[MAX_FILENAME_SIZE];
        int fileCount;

        sscanf(line, "%s - %d", fileName, &fileCount);
        strcpy(fileNames[fileIndex], fileName);
        fileCounts[fileIndex] = fileCount;
        fileIndex++;
    }

    // Search for the target filename
    int targetIndex = -1;
    for (int i = 0; i < fileIndex; i++)
    {
        if (strcmp(fileNames[i], targetFile) == 0)
        {
            targetIndex = i; // Get index of target file
            break;
        }
    }

    if (targetIndex == -1)
    {
        printf("Error: File %s not found\n", targetFile);
        fclose(file);
        return;
    }

    char *dot = strrchr(fileNames[targetIndex], '.'); // Find last '.' to split extension
    if (!dot)
    {
        printf("Error: No file extension found in %s\n", fileNames[targetIndex]);
        fclose(file);
        return;
    }

    char baseName[MAX_FILENAME_SIZE];
    strncpy(baseName, fileNames[targetIndex], dot - fileNames[targetIndex]);
    baseName[dot - fileNames[targetIndex]] = '\0';

    printf("%s(%d)%s\n", baseName, fileCounts[targetIndex], dot);

    char formattedFileName[256];
    snprintf(formattedFileName, sizeof(formattedFileName), "%s(%d)%s", baseName, fileCounts[targetIndex], dot);

    fileCounts[targetIndex]++;
    rewind(file);
    for (int i = 0; i < fileIndex; i++)
    {
        fprintf(file, "%s - %d\n", fileNames[i], fileCounts[i]);
    }

    fclose(file);

    // Open the userName/userName.config file to append the new entry
    char configFilePath[256];
    snprintf(configFilePath, sizeof(configFilePath), "%s/%s.config", userName, userName);
    FILE *configFile = fopen(configFilePath, "a"); // Open in append mode
    if (!configFile)
    {
        printf("Error: Could not open config file %s\n", configFilePath);
        return;
    } 
    
    fprintf(configFile, "%s - %d\n", formattedFileName, fileCounts[targetIndex]);
    fclose(configFile);

    receive_replace_able_file_content(clientSocket, userName, formattedFileName);
}

void handleFileExists(int clientSocket, const char *fileName, const char *userName, const char *ActualFile)
{
    const char *fileExistsMsg = "File already exists.";
    send(clientSocket, fileExistsMsg, strlen(fileExistsMsg), 0);

    char clientResponse[2];
    recv(clientSocket, clientResponse, 2, 0);

    if (clientResponse[0] == '1')
    {
        printf("File '%s' replaced for user: %s\n", fileName, userName);
        receive_updated_file_content(clientSocket, userName);
        return;
    }
    else if (clientResponse[0] == '0')
    {
        printf("File '%s' will not be replaced for user: %s\n", fileName, userName);
        updateFileCount(clientSocket, userName, ActualFile);
        ;
    }
    else
    {
        printf("Invalid input from client.\n");
    }
}

/* =====   Parse File for Checking User's Storage And Info      ======  */

int parseFileAfterAsterisk(const char *userName, char fileNames[MAX_FILES][MAX_FILENAME_SIZE], int *fileCount)
{
    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s.config", userName, userName);

    int fileDescriptor = open(filePath, O_RDONLY);
    if (fileDescriptor < 0)
    {
        perror("Error opening config file");
        return -1;
    }

    char buffer[MAX_SIZE];
    ssize_t bytesRead;
    int foundAsterisk = 0;

    int fileSizes[MAX_FILES];
    *fileCount = 0;

    while ((bytesRead = read(fileDescriptor, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytesRead] = '\0';

        if (!foundAsterisk)
        {
            char *asteriskPos = strchr(buffer, '*');
            if (asteriskPos != NULL)
            {
                foundAsterisk = 1;
                char *newlinePos = strchr(asteriskPos, '\n');
                if (newlinePos != NULL)
                {
                    newlinePos++;
                    memmove(buffer, newlinePos, bytesRead - (newlinePos - buffer));
                    bytesRead -= (newlinePos - buffer);
                    buffer[bytesRead] = '\0';
                }
            }
        }

        if (foundAsterisk)
        {
            char *linePtr = buffer;
            while (*linePtr != '\0' && *fileCount < MAX_FILES)
            {
                char *spacePtr = strchr(linePtr, ' ');
                if (spacePtr == NULL)
                {
                    break;
                }

                *spacePtr = '\0';
                strncpy(fileNames[*fileCount], linePtr, MAX_FILENAME_SIZE - 1);
                fileNames[*fileCount][MAX_FILENAME_SIZE - 1] = '\0';

                linePtr = spacePtr + 1;

                char *dashPtr = strchr(linePtr, '-');
                if (dashPtr == NULL)
                {
                    break;
                }

                linePtr = dashPtr + 1;
                while (*linePtr == ' ')
                    linePtr++;

                char *endPtr;
                fileSizes[*fileCount] = (int)strtol(linePtr, &endPtr, 10);

                if (endPtr == linePtr || fileSizes[*fileCount] <= 0)
                {
                    fprintf(stderr, "Error parsing file size\n");
                    close(fileDescriptor);
                    return -5;
                }

                (*fileCount)++;

                linePtr = strchr(endPtr, '\n');
                if (linePtr != NULL)
                {
                    linePtr++;
                }
                else
                {
                    break;
                }
            }
        }
    }

    if (bytesRead < 0)
    {
        perror("Error reading config file");
        close(fileDescriptor);
        return -2;
    }

    close(fileDescriptor);

    printf("Files and Sizes:\n");
    for (int i = 0; i < *fileCount; i++)
    {
        printf("File: %s\n", fileNames[i]);
    }

    int totalSize = calculateSumOfSizes(fileSizes, *fileCount);
    printf("\nTotal Size of All Files: %d bytes\n", totalSize);

    return totalSize;
}

void write_FileInfo_to_user_Config(int clientSocket, const char *userName, const char *fileName, size_t fileSize)
{
    char fileNames[MAX_FILES][MAX_FILENAME_SIZE];
    int fileCount = 0;

    int totalSize = parseFileAfterAsterisk(userName, fileNames, &fileCount);

    if (totalSize < 0)
    {
        fprintf(stderr, "Error parsing config file. Error code: %d\n", totalSize);
        const char *errorMsg = "Error parsing config file.";
        send(clientSocket, errorMsg, strlen(errorMsg) + 1, 0);
        return;
    }

    printf("Total size: %d\n", totalSize);

    if (totalSize + fileSize > MAX_STORAGE)
    {
        const char *outOfSpaceMsg = "Out of space.";
        send(clientSocket, outOfSpaceMsg, strlen(outOfSpaceMsg) + 1, 0);
        printf("Out of Space\n");
        return;
    }

    char filePath[1024];
    snprintf(filePath, sizeof(filePath), "%s/%s.config", userName, userName);

    char fileListPath[1024];
    snprintf(fileListPath, sizeof(fileListPath), "%s/%s_fileList.config", userName, userName);

    char userFilePath[1024];
    snprintf(userFilePath, sizeof(userFilePath), "%s/%s", userName, fileName);

    if (access(userFilePath, F_OK) == 0)
    {
        // const char *fileExistsMsg = "File already exists.";
        // send(clientSocket, fileExistsMsg, strlen(fileExistsMsg), 0);
        // printf("File '%s' already exists for user: %s\n", fileName, userName);
        // return;
        printf("File '%s' already exists for user\n", fileName);
        handleFileExists(clientSocket, filePath, userName, fileName);
        return;
    }

    int fileDescriptor = open(filePath, O_WRONLY | O_APPEND);
    if (fileDescriptor < 0)
    {
        perror("Error opening config file for appending");
        const char *errorMsg = "Error updating config file.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    if (dprintf(fileDescriptor, "%s - %zu\n", fileName, fileSize) < 0)
    {
        perror("Error writing to config file");
        const char *errorMsg = "Error writing to config file.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
    }
    else
    {
        const char *successMsg = "File Data updated.";
        send(clientSocket, successMsg, strlen(successMsg) + 1, 0);
        printf("File Data updated for user: %s\n", userName);
    }

    close(fileDescriptor);

    // Write to the fileList.config file
    int fileListDescriptor = open(fileListPath, O_WRONLY | O_APPEND);
    if (fileListDescriptor < 0)
    {
        perror("Error opening file list for appending");
        const char *errorMsg = "Error updating file list.";
        send(clientSocket, errorMsg, strlen(errorMsg), 0);
        return;
    }

    if (dprintf(fileListDescriptor, "%s - 1\n", fileName) < 0)
    {
        perror("Error writing to file list");
        const char *errorMsg = "Error writing to file list.";
        send(clientSocket, errorMsg, strlen(errorMsg) + 1, 0);
    }
    else
    {
        printf("FileList updated with '%s - 1' for user: %s\n", fileName, userName);
    }

    close(fileListDescriptor);

    receiveFileFromClient(clientSocket, userName);
}
