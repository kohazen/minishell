#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // fork, execvp, chdir
#include <sys/types.h>
#include <sys/wait.h>   // waitpid
#include <signal.h>     // signal handling

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64 

void sigint_handler(int sig) {
    printf("\nmyshell> ");
    fflush(stdout); // Ensures the prompt appears immediately
}

void sigchld_handler(int sig) { 
    // WNOHANG ensures the shell doesn't block if there are no dead children found.
    while (waitpid(-1, NULL, WNOHANG) > 0); 
}

int main() {
    char input[MAX_CMD_LEN];
    char *args[MAX_ARGS]; 
    int background = 0;

    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler); 

    while (1) {
        printf("myshell> ");
        if (fgets(input, MAX_CMD_LEN, stdin) == NULL) break;

        // Clean up the newline character and ensure input is not empty
        input[strcspn(input, "\n")] = 0;

        // Parsing
        int i = 0;
        char *token = strtok(input, " ");
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL; 

        if (args[0] == NULL) continue;

        
        // exit and cd 
        if (strcmp(args[0], "exit") == 0) exit(0);
        
        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) fprintf(stderr, "cd: missing argument\n");
            else if (chdir(args[1]) != 0) perror("cd failed");
            continue;
        }

        // external commands
        pid_t pid = fork(); 

        if (pid == 0) {
            // CHILD PROCESS 
            if (execvp(args[0], args) < 0) {
                perror("Command not found");
                exit(1);
            }
        } else if (pid > 0) {
            // PARENT PROCESS 
            if (!background) {
                waitpid(pid, NULL, 0); 
            } else {
                waitpid(pid, NULL, 0); 
            }
        } else {
            perror("Fork failed");
        }
    }
    return 0;
}