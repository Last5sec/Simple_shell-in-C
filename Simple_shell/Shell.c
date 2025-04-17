#include "shell.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

// Entry point of the shell program
int main(int argc, char *argv[]) {
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));  // Initialize the sigaction structure to zero
    sig.sa_handler = Terminate;  // Set the signal handler to Terminate()

    // Set up signal handling for SIGINT (Ctrl+C) to allow graceful termination
    if (sigaction(SIGINT, &sig, NULL) == -1) return 1;

    // If no argument is provided, enter shell loop, otherwise execute the shell script
    if (argc != 2) {
        shell_loop();  // Start the interactive shell loop
        return 1;
    }

    // Execute a shell script if the user passes a file name as an argument
    if (argv != NULL) {
        execute_shell(argv[1]);  // Execute the script passed in argv[1]
    } else {
        printf("Either an invalid or empty command has been entered\n");
    }

    return 0;
}

// The main shell loop that continuously waits for user input
void shell_loop() {
    char *wholeCommand;  // Full command entered by the user
    char **individualCommands;  // Command tokens after splitting
    int count = 0;  // History counter to store commands

    // Infinite loop for shell prompt and command execution
    do {
        printf("\033[1;31mGroup-31@:\033[0m~$");  // Print a colored shell prompt
        wholeCommand = read_input();  // Read user input from stdin
        char *whcmd = strdup(wholeCommand);  // Duplicate command for history tracking

        pid_t pid;
        struct timespec start_time;
        clock_gettime(CLOCK_REALTIME, &start_time);  // Record the start time of the command

        // Handle the "history" command to display previous commands
        if (strcmp(wholeCommand, "history\n") == 0) {
            history(count);  // Display command history
            free(whcmd);  // Free the duplicated command memory
            continue;  // Skip to the next iteration of the loop
        }

        // Handle the "exit" command to quit the shell
        if (strcmp(wholeCommand, "exit\n") == 0) {
            free(whcmd);  // Free the command memory
            clean_up_memory(wholeCommand, individualCommands);  // Clean up memory
            break;  // Exit the shell loop
        }

        // Check if the command contains pipes ('|')
        char *pipe_check = strchr(wholeCommand, '|');
        if (pipe_check == NULL) {  // No pipes, execute single command
            individualCommands = split_tokens(wholeCommand, " \n");  // Split command by spaces
            pid = launch_command(individualCommands);  // Launch the command
        } else {  // Command contains pipes, handle multiple piped commands
            individualCommands = split_tokens(wholeCommand, "|");  // Split by pipe character
            pid = launch_with_pipes(individualCommands);  // Execute the piped commands
        }

        // Add command to history only if it's valid and not "history"
        if (pid != -1 && strcmp(wholeCommand, "history\n") != 0) {
            command_history[count].wholeCommand = strdup(whcmd);  // Store the command
            command_history[count].start_time = start_time;  // Store the start time
            command_history[count].pid = pid;  // Store the PID of the command
            count++;  // Increment the history counter
        }

        free(whcmd);  // Free the duplicated command
        clean_up_memory(wholeCommand, individualCommands);  // Clean up memory after each iteration
    } while (1);  // Continue the loop indefinitely
}

// Reads input from the user
char *read_input() {
    char *line = NULL;  // Pointer for the input line
    size_t bufsize = 0;  // Size for getline to dynamically allocate memory
    ssize_t read_chars = getline(&line, &bufsize, stdin);  // Read a line from stdin
    handle_error(read_chars);  // Handle error if getline fails
    return line;  // Return the user input line
}

// Tokenizes the command line input based on a delimiter
char **split_tokens(char *line, const char *delim) {
    int count = 0;
    char *line_copy = strdup(line);  // Duplicate the command line
    char *token = strtok(line_copy, delim);  // Tokenize the line

    // Count the number of tokens
    while (token != NULL) {
        count++;
        token = strtok(NULL, delim);
    }

    // Allocate memory for tokens
    char **tokens = malloc((count + 1) * sizeof(char *));
    if (tokens == NULL) exit(0);  // Error handling for malloc failure

    token = strtok(line, delim);  // Reinitialize strtok to tokenize the original line
    for (int i = 0; token != NULL; i++) {
        tokens[i] = strdup(token);  // Copy each token
        token = strtok(NULL, delim);
    }
    tokens[count] = NULL;  // Set the last token as NULL

    free(line_copy);  // Free the duplicated string
    return tokens;  // Return the array of tokens
}

// Launch a command that contains pipes
pid_t launch_with_pipes(char **commands) {
    if (commands[0] == NULL) {  // Handle empty command
        printf("An empty command has been entered\n");
        return -1;
    }
    
    int num_commands = count_commands(commands);  // Count the number of piped commands
    if (num_commands == 0) return -1;

    return execute_with_pipes(num_commands, commands);  // Execute the piped commands
}

// Execute commands connected by pipes
pid_t execute_with_pipes(int argc, char **argv) {
    int fd[2], stand_in = 0, status;
    pid_t pid;
    pid_t pids[argc];  // Array to hold PIDs of the child processes

    for (int i = 0; i < argc; i++) {
        char **cmd = split_tokens(argv[i], " \n");  // Split each command into tokens
        pipe(fd);  // Create a pipe for inter-process communication

        if ((pid = fork()) == 0) {  // Child process
            if (i != 0) dup2(stand_in, 0);  // Redirect input from the previous command's output
            if (i != argc - 1) dup2(fd[1], 1);  // Redirect output to the next command's input
            close(fd[0]);  // Close the read end of the pipe
            execvp(cmd[0], cmd);  // Execute the command
            perror("execvp failed");  // Error handling if execvp fails
            exit(0);
        } else if (pid < 0) {
            perror("fork failed");  // Error handling if fork fails
            return -1;
        } else {
            pids[i] = pid;  // Store the PID of the child process
            close(fd[1]);  // Close the write end of the pipe in the parent
            stand_in = fd[0];  // Use the read end of the pipe for the next command
        }

        clean_up_memory(NULL, cmd);  // Free memory for each command
    }

    // Wait for all child processes to complete
    for (int i = 0; i < argc; i++) waitpid(pids[i], &status, 0);

    return pids[0];  // Return the PID of the first command
}

// Launch a single command without pipes
pid_t launch_command(char **commands) {
    if (commands[0] == NULL) {  // Handle empty command
        printf("An empty command has been entered\n");
        return -1;
    }

    int num_commands = count_commands(commands);  // Count the number of tokens
    int is_background = (strcmp(commands[num_commands - 1], "&") == 0);  // Check if command should run in the background

    if (is_background) {
        commands[num_commands - 1] = NULL;  // Remove '&' from the command tokens
    }

    pid_t pid = fork();  // Fork a child process to execute the command

    if (pid == 0) {  // Child process
        if (is_background) {
            // Redirect output to /dev/null if running in background
            freopen("/dev/null", "w", stdout);  // Ignore standard output for background tasks
        }
        execvp(commands[0], commands);  // Execute the command
        perror("execvp failed");  // Handle error if execvp fails
        exit(0);
    } else if (pid > 0) {  // Parent process
        if (!is_background) {
            int status;
            waitpid(pid, &status, 0);  // Wait for the child process to complete (foreground task)
        } else {
            // Notify the user that the process is running in the background
            printf("Process with PID %d running in the background...\n", pid);
        }
    } else {
        perror("fork failed");  // Handle fork failure
        return -1;
    }

    return pid;  // Return the PID of the child process
}


// Count the number of tokens/commands in the array
int count_commands(char **commands) {
    if (commands == NULL) {
        printf("There are no commands to execute\n");
        return 0;
    }
    int num_commands = 0;
    while (commands[num_commands] != NULL) {
        num_commands++;
    }
    return num_commands;
}

// Signal handler for SIGINT (Ctrl+C)
void Terminate(int signum) {
    if (signum == SIGINT) {
        printf("\nTerminating the shell...\n");
        exit(1);  // Exit the shell when Ctrl+C is pressed
    }
}

// Display the history of executed commands
void history(int count) {
    printf("\nCommand History:\n");
    for (int i = 0; i < count; i++) {
        printf("[%d]:\n", i + 1);  // Command number
        printf("Command: %s\n", command_history[i].wholeCommand);  // Display the command
        printf("PID: %d\n", command_history[i].pid);  // Display the PID
        
        // Format the start time (without decimals for seconds)
        char start_time_formatted[20];
        strftime(start_time_formatted, sizeof(start_time_formatted), "%d-%m-%Y %H:%M:%S", localtime(&command_history[i].start_time.tv_sec));
        
        // Print the start time without any nanoseconds
        printf("Start Time: %s\n\n", start_time_formatted);
    }
}

// Clean up dynamically allocated memory for a line and token array
void clean_up_memory(char *line, char **commands) {
    if (line) free(line);  // Free the input line
    if (commands) { 
        for (int i = 0; commands[i] != NULL; i++) {
            free(commands[i]);  // Free each command token
        }
        free(commands);  // Free the array of tokens
    }
}

// Execute a shell script from a file
void execute_shell(const char *script_filename) {
    FILE *scriptFile = fopen(script_filename, "r");  // Open the script file for reading
    if (scriptFile == NULL) return;  // Return if file couldn't be opened
    
    char command[8192] = "";  // Buffer to hold the entire script
    char line[2048];          // Buffer for each line in the script

    // Read the file line by line and concatenate into a single command
    while (fgets(line, sizeof(line), scriptFile) != NULL) {
        strcat(command, line);  // Append each line to the command buffer
    }
    
    fclose(scriptFile);  // Close the file after reading

    // Execute the full script as one command
    if (strlen(command) > 0) {
        system(command);  // Use system() to execute the concatenated script
    }
}

// Handle errors while reading input
void handle_error(ssize_t b) {
    if (b == -1) {
        exit(0);  // Exit if there is an error in reading input
    }
}

// Structure to store command history
struct cmd command_history[MAX_HISTORY_SIZE];
