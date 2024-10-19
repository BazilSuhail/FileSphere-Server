# Server Program in C

This directory contains a C program source file named `server.c`. The program implements a server-side application that allows clients to manage their uploaded files and check their sizes. It also includes basic user authentication.

To build and execute this C program, use the provided Makefile, which will compile the source code into an executable named `main.exe`.

## Files

- **`server.c`**: The source code for the server-side application.
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
