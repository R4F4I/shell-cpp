# Custom C++ Command Line Shell

A lightweight, Unix-like command-line shell written in C++. This project demonstrates core Operating System concepts including process creation, synchronization, system calls, and signal handling.

## Features
* **Standard Execution:** Runs standard Linux commands (e.g., `ls -l`, `grep`) with automatic `$PATH` resolution.
* **Built-in Commands:** Native support for `cd`, `pwd`, `history`, and `exit`.
* **Background Jobs:** Supports running commands in the background by appending `&` (e.g., `sleep 10 &`).
* **Environment Variables:** Expands variables prefixed with `$` (e.g., `echo $USER`).
* **Signal Handling:** Safely handles `Ctrl+C` (`SIGINT`) by interrupting the running child process without killing the main shell.
* **Custom UI:** Features an ANSI-colored prompt and startup banner.

## Compilation and Execution

This project requires a Linux environment (or WSL) and the GCC compiler. It uses standard POSIX libraries.

**1. Compile the code:**
```bash
g++ -Wall -std=c++11 shell.cpp -o myshell
```

**2. Run the shell:**
```bash
./myshell
```

## How It Works (Core Architecture)

This shell is built on the classic **REPL** (Read, Evaluate, Print, Loop) architecture and relies heavily on Linux system calls:

1. **Reading & Parsing Input:** The shell waits for user input using `std::getline`. The input is tokenized into a vector of strings. During parsing, the shell checks for environment variables (using `getenv()`) and the background execution flag (`&`).
2. **Handling Built-ins:** Commands like `cd` and `exit` must modify the shell's own state. Therefore, they are intercepted and executed directly by the parent process using system calls like `chdir()`.
3. **Process Spawning (`fork`):** For standard commands, the shell calls `fork()`. This creates an exact, separate copy of the shell process (the "Child"). 
4. **Execution (`execvp`):** Inside the child process, `execvp()` is called. This replaces the child's memory space with the binary of the requested program (like `ls` or `cat`). It automatically searches the system's `$PATH` to find the executable.
5. **Synchronization (`waitpid`):** Meanwhile, the parent process (the shell) calls `waitpid()`. It sleeps and waits for the child process to finish executing. If the user requested background execution (`&`), the parent skips this wait step and immediately prints the next prompt, periodically cleaning up finished background tasks (zombie processes) using `WNOHANG`.
