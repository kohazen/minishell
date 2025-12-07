#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CMD_LEN 1024

int main() {
    char input[MAX_CMD_LEN];

    while (1) {
        printf("myshell> ");
        // Read input
        if (fgets(input, MAX_CMD_LEN, stdin) == NULL) {
            break; 
        }

        // Remove newline character
        input[strcspn(input, "\n")] = 0;

        // Exit command (basic)
        if (strcmp(input, "exit") == 0) {
            break;
        }
    }
    return 0;
}
