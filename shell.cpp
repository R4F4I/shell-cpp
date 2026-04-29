#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <limits.h>

// Global history vector to store past commands
std::vector<std::string> command_history;

// Signal handler for Ctrl+C (SIGINT)
// This prevents the shell from exiting when the user types Ctrl+C
void sigint_handler(int sig) {
    std::cout << "\n";
    // We don't reprint the prompt here because the main loop will handle it
}

// Function to clean up background processes (preventing zombies)
void reap_zombies() {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Function to print the colored shell prompt
// Add this new function to print a cool banner on startup
void print_banner() {
    std::cout << "\033[2J\033[1;1H";
    std::cout << "\033[1;36m"; // Bold Cyan
    std::cout << "====================================================\n";
    std::cout << "   🚀 WELCOME TO THE CUSTOM OS SHELL (C++ Edition)  \n";
    std::cout << "====================================================\n";
    std::cout << " Type 'help', 'history', or standard Linux commands \n";
    std::cout << "====================================================\n";
    std::cout << "\033[0m";    // Reset to default
}

// Replace your old print_prompt with this completely custom one
void print_prompt() {
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, sizeof(hostname));
    
    char* username = getlogin();
    if (!username) username = getenv("USER");

    // Format: [MyShell] user@host : /current/dir ❯
    // Colors: Magenta [MyShell], Yellow user@host, Cyan dir, Red Arrow
    std::cout << "\033[1;35m[MyShell]\033[0m " 
              << "\033[1;33m" << (username ? username : "user") << "@" << hostname << "\033[0m "
              << ":: \033[1;36m" << cwd << "\033[0m "
              << "\033[1;31m❯\033[0m ";
    std::cout.flush();
}
int main() {
    // 1. Setup Signal Handling: Ignore Ctrl+C in the parent shell
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // Restart interrupted system calls like getline
    sigaction(SIGINT, &sa, NULL);

    print_banner();
    
    std::string input;

    // The REPL Loop (Read, Evaluate, Print, Loop)
    while (true) {
        reap_zombies(); // Clean up any finished background jobs
        print_prompt();

        // 2. Read input
        if (!std::getline(std::cin, input)) {
            // Handle EOF (Ctrl+D)
            std::cout << "\nexit\n";
            break;
        }

        if (input.empty()) continue;

        // Save to history (Additional Feature)
        command_history.push_back(input);

        // 3. Parse input
        std::vector<std::string> tokens;
        std::stringstream ss(input);
        std::string token;
        bool is_background = false;

        while (ss >> token) {
            // Environment Variable Expansion (Additional Feature)
            if (token[0] == '$' && token.length() > 1) {
                const char* env_val = getenv(token.c_str() + 1); // Skip the '$'
                if (env_val) {
                    token = env_val;
                } else {
                    token = ""; // If var doesn't exist, replace with empty string
                }
            }
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }

        if (tokens.empty()) continue;

        // Check for background execution '&' (Additional Feature)
        if (tokens.back() == "&") {
            is_background = true;
            tokens.pop_back(); // Remove the '&' from arguments
        } else if (tokens.back().back() == '&') {
            // Handle case where user typed "command&" without space
            is_background = true;
            tokens.back().pop_back();
            if (tokens.back().empty()) tokens.pop_back();
        }

        // 4. Handle Built-in Commands (Additional Feature)
        if (tokens[0] == "exit") {
            break;
        } 
        else if (tokens[0] == "cd") {
            if (tokens.size() > 1) {
                if (chdir(tokens[1].c_str()) != 0) {
                    perror("cd failed");
                }
            } else {
                // cd with no args goes to HOME
                const char* home = getenv("HOME");
                if (home) chdir(home);
            }
            continue;
        }
        else if (tokens[0] == "pwd") {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                std::cout << cwd << "\n";
            }
            continue;
        }
        else if (tokens[0] == "history") {
            for (size_t i = 0; i < command_history.size(); ++i) {
                std::cout << i + 1 << "  " << command_history[i] << "\n";
            }
            continue;
        }

        // Convert std::vector<std::string> to char* array for execvp
        std::vector<char*> args;
        for (auto& t : tokens) {
            args.push_back(const_cast<char*>(t.c_str()));
        }
        args.push_back(nullptr); // execvp requires a NULL-terminated array

        // 5. Execute Commands (Mandatory)
        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
        } 
        else if (pid == 0) {
            // --- CHILD PROCESS ---
            // Re-enable default Ctrl+C behavior for the running command
            signal(SIGINT, SIG_DFL);
            
            // Execute the command
            if (execvp(args[0], args.data()) < 0) {
                std::cerr << args[0] << ": command not found\n";
                exit(1); // Exit child if command fails
            }
        } 
        else {
            // --- PARENT PROCESS ---
            if (!is_background) {
                // Wait for the child to finish
                int status;
                waitpid(pid, &status, 0);
            } else {
                // Background Execution
                std::cout << "[Background job started] PID: " << pid << "\n";
            }
        }
    }

    return 0;
}
