#ifndef SHELL_H
#define SHELL_H
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_HISTORY_SIZE 1000
#define MAX_LINE_LENGTH 1000

struct cmd {
    char *name;
    char *wholeCommand;
    pid_t pid;
    struct timespec start_time;
};

extern struct cmd command_history[MAX_HISTORY_SIZE];

char *read_input();
char **split_tokens(char *line , const char *delim);
int count_commands(char **commands);
pid_t launch_command(char **commands);
pid_t execute_with_pipes(int argc, char **argv);
pid_t launch_with_pipes(char **commands);
void shell_loop();
void history();
void Terminate(int signum);
void clean_up_memory(char *line, char **commands);
void execute_shell(const char *script_filename);
void handle_error(ssize_t b);
#endif