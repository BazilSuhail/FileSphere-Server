This folder has a C program source code named `server.c`, This code is for a server implemented in C for a client to look after his files uploaded to check for sizes covering a particular user covering basic authentication in C. For this C code use make file to build it and then the C program will be compiled into `main.exe`. 

## Files Made
- `server.c`: This is the source file containing the C source code of actual server.
- `Makefile`: This file is used for building, cleaning and running the project.

## How to build and Execute the code

### About Execution and Build
The following commands are used to compile, clean, and run the executable of this C program:

- `make`: Compiles/builds the program by generating the executable.
- `make clean`: Removes the existing executable and any compiled files.
- `make run`: Compiles the program if the executable is not present or if the source file(s) have been updated, otherwise it directly runs the program.
- `make clean run`: First removes the old executable, then compiles the program, and finally runs it.

### Tools Required
- `gcc`: A widely used C compiler, part of the GNU Compiler Collection.
- `make`: A build tool used to manage and automate the process of compiling code.
- `libc6-dev`: The development libraries and headers for the GNU C Library, which are essential for compiling C programs

### Setup Instructions
Before building the program, ensure that build tools (compiler, make, libc6-dev) are installed on your system. On a Debian-based system (e.g., Ubuntu), you can install them using the following commands:

```sh
sudo apt-get update
sudo apt-get install build-essential
```
