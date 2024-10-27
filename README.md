
# Server Program in C ###(with Queue-Based Reader-Writer Lock)
<hr/>
This project implements a multi-client server that uses a queue-based reader-writer lock to manage concurrent read and write operations for users. It uses threads, socket programming, and synchronization mechanisms to handle multiple clients efficiently. The server supports different tasks (such as upload, delete, update, download, and view) based on user requests, with synchronization to prevent data conflicts.

## Features

### 1. Server Setup and Connection Management
- **Socket Initialization**: Creates a TCP server that listens on a specified port (`PORT`).
- **Client Connections**: Handles up to a maximum number of clients (`MAX_CONNECTIONS`) concurrently.
- **Threaded Client Handling**: Spawns a new thread for each client connection to handle client-specific tasks independently.

### 2. User Management
- **User Information Storage**: Stores and manages user-specific data (e.g., username, read/write counts, and request queues).
- **User Initialization**: Creates a new `UserInfo` entry for a client if the user is not already connected.
- **Global User List**: Maintains a list of active users, limited by `MAX_CONNECTIONS`.

### 3. Queue-Based Reader-Writer Synchronization
- **Request Queue**: Manages a queue of requests (`READ` or `WRITE`) for each user.
- **Read Operations**: Allows multiple clients to read simultaneously, provided no write is in progress.
- **Write Operations**: Allows only one client to write at a time, blocking reads and other writes until the write completes.
- **Queue Processing**: Processes requests in a first-come, first-served order for fairness.

### 4. Client Task Management
- **Task Options**: Supports a range of operations based on client requests:
  - **Upload/Delete/Update** (Write operations): Requires exclusive access to user data.
  - **Download/View** (Read operations): Allows shared access unless a write is ongoing.
- **Read-Write Execution**: Manages read and write operations with functions `startRead`, `finishRead`, `startWrite`, and `finishWrite` to ensure safe concurrent access.

### 5. Authentication/Registration
- **Registration Logic**: Registration of the user using his name and password(e.g., storing of credentials like name and passwords).
- **Authentication Logic**: Authentication of user using his name and password with which he registered his account(e.g., name and password checks).
- **Username Retrieval**: Retrieves the username for each client upon connection.

### 6. Synchronization and Thread Safety
- **Mutex Locks and Condition Variables**: Uses mutexes and condition variables to ensure safe access to shared data.
- **Global Connection Control**: Limits the maximum number of client connections using a global mutex and condition variable.

### 6. Functionalites Offered to User
- **Upload**: Upload files to the server. Files are saved in the User's directory on the server, the server alos asks user to replace if the file exists otherwise will create a copy automatically.
- **Download** Download files from the server. The requested file is sent from the server to the client.
- **View** View file details, such as file name, size, and creation date, if available.
- **Delete** Delete files from the server. The specified file is removed from the server’s storage.
- **Update** Update or replace existing files on the server with a new version.

<hr/>

To build and execute this C program, use the provided Makefile, which will compile the source code into an executable named `main.exe`.
## Files
- **`server.c`**: The source code for the server-side application.
- **`helper_authentication.c`**: The source code for the registering, authenticating and processing user's specific tasks.
- **`helper_queries.c`**: The source code for the functionalities offered to the client.
- **`helper_synchronization.c`**: The source code to implement or ensure synchronization between users/threads.
- **`helper_parsingConfig.c`**: The source code for pasrsing User's `<userName>.condig` and `<userName>_fileList.config` file for checking available space and file records.
- **`Makefile`**: Automates the building, cleaning, and execution of the project.

## How to Build and Execute the Code

### Build and Execution Commands

Use the following commands to compile, clean, and run the executable:

- **`make`**: Compiles the program and generates the `main.exe` executable.
- **`make clean`**: Removes the existing executable and any intermediate compiled files.
- **`make run`**: Compiles the program if necessary (i.e., if the executable is missing or the source has changed) and then runs the program.
- **`make clean run`**: Cleans any previous builds, compiles the program, and then runs the executable.

### Tools Required

- **`gcc`**: A popular C compiler, part of the GNU Compiler Collection.
- **`make`**: A tool for automating the build process and managing dependencies.
- **`libc6-dev`**: Development libraries and headers for the GNU C Library, required for compiling C programs.

### Setup Instructions

Before building the program, ensure that all necessary tools (compiler, make, libc6-dev) are installed on your system. On a Debian-based system (e.g., Ubuntu), you can install them using the following commands:

```bash
sudo apt-get update
sudo apt-get install build-essential
```
Once the tools are installed, navigate to the directory containing the `Makefile` and run the desired command (e.g., `make` or `make run`).

---
