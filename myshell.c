
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define MAX_TOKENS 256
#define MAX_ARGV 128
#define PROMPT "myshell> "

typedef struct {
    char *argv[MAX_ARGV];   // NULL terminated
    char *infile;           // filename for '<' or NULL
    char *outfile;          // filename for '>' or NULL
    int background;         // 1 if trailing &
} Command;

/* Helper: print error and continue */
static void perror_continue(const char *msg) {
    perror(msg);
}

/* Signal handling: parent ignores SIGINT; children will get default */
static void sigint_handler_parent(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\n", 1); /* newline on Ctrl-C */
}

/* Trim leading/trailing whitespace in-place */
static char *trim(char *s) {
    if (!s) return s;
    while (*s && (*s == ' ' || *s == '\t' || *s == '\n')) s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' || *end == '\n')) {
        *end = '\0'; end--;
    }
    return s;
}

/*
 * Tokenizer that respects single and double quotes.
 * Returns number of tokens and fills tokens[] array with allocated strings.
 * Caller must free each tokens[i].
 */
static int tokenize(const char *line, char **tokens, int max_tokens) {
    const char *p = line;
    int ntok = 0;

    while (*p && ntok < max_tokens) {
        /* skip whitespace */
        while (*p == ' ' || *p == '\t' || *p == '\n') p++;
        if (!*p) break;

        if (*p == '"' || *p == '\'') {
            char quote = *p++;
            const char *start = p;
            while (*p && *p != quote) {
                if (*p == '\\' && quote == '"' && p[1]) p++; /* allow escape in double quotes */
                p++;
            }
            size_t len = p - start;
            char *tok = malloc(len + 1);
            if (!tok) { fprintf(stderr,"malloc failed\n"); exit(1); }
            strncpy(tok, start, len);
            tok[len] = '\0';
            tokens[ntok++] = tok;
            if (*p == quote) p++;
        } else {
            /* symbol tokens like > < | & are separate tokens; otherwise accumulate until whitespace */
            if (*p == '>' || *p == '<' || *p == '|' || *p == '&') {
                char *tok = malloc(2);
                tok[0] = *p;
                tok[1] = '\0';
                tokens[ntok++] = tok;
                p++;
            } else {
                const char *start = p;
                while (*p && !(*p == ' ' || *p == '\t' || *p == '\n' ||
                              *p == '>' || *p == '<' || *p == '|' || *p == '&')) {
                    p++;
                }
                size_t len = p - start;
                char *tok = malloc(len + 1);
                if (!tok) { fprintf(stderr,"malloc failed\n"); exit(1); }
                strncpy(tok, start, len);
                tok[len] = '\0';
                tokens[ntok++] = tok;
            }
        }
    }
    return ntok;
}

/* Free token array */
static void free_tokens(char **tokens, int n) {
    for (int i = 0; i < n; ++i) free(tokens[i]);
}

/* Parse an array of tokens (ntok) into a Command structure.
 * It consumes tokens from index start..end-1 and fills cmd.
 * Returns 0 on success, -1 on parse error.
 */
static int parse_command_from_tokens(char **tokens, int start, int end, Command *cmd) {
    memset(cmd, 0, sizeof(Command));
    int argi = 0;
    for (int i = start; i < end; ++i) {
        char *t = tokens[i];
        if (strcmp(t, "<") == 0) {
            if (i+1 >= end) { fprintf(stderr,"syntax error: expected filename after '<'\n"); return -1;}
            cmd->infile = strdup(tokens[++i]);
        } else if (strcmp(t, ">") == 0) {
            if (i+1 >= end) { fprintf(stderr,"syntax error: expected filename after '>'\n"); return -1;}
            cmd->outfile = strdup(tokens[++i]);
        } else if (strcmp(t, "&") == 0) {
            // background token — only valid if last token or we interpret as trailing
            if (i != end-1) {
                fprintf(stderr, "syntax warning: '&' not at end — treating as background\n");
            }
            cmd->background = 1;
        } else {
            if (argi >= MAX_ARGV-1) { fprintf(stderr,"too many args\n"); return -1; }
            cmd->argv[argi++] = strdup(t);
        }
    }
    cmd->argv[argi] = NULL;
    return 0;
}

/* Free command internals */
static void free_command(Command *c) {
    for (int i = 0; c->argv[i]; ++i) free(c->argv[i]);
    if (c->infile) free(c->infile);
    if (c->outfile) free(c->outfile);
}

/* Execute a single (non-piped) command */
static int execute_single(Command *cmd) {
    if (!cmd->argv[0]) return 0;

    /* Builtins */
    if (strcmp(cmd->argv[0], "exit") == 0) {
        exit(0);
    }
    if (strcmp(cmd->argv[0], "cd") == 0) {
        const char *dir = cmd->argv[1] ? cmd->argv[1] : getenv("HOME");
        if (chdir(dir) != 0) perror_continue("cd");
        return 0;
    }

    pid_t pid = fork();
    if (pid < 0) { perror_continue("fork"); return -1; }
    if (pid == 0) {
        /* child: restore default signal handlers so Ctrl-C works inside child */
        signal(SIGINT, SIG_DFL);

        /* handle redirections */
        if (cmd->infile) {
            int fd = open(cmd->infile, O_RDONLY);
            if (fd < 0) { fprintf(stderr,"Failed to open %s: %s\n", cmd->infile, strerror(errno)); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        if (cmd->outfile) {
            int fd = open(cmd->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd < 0) { fprintf(stderr,"Failed to open %s: %s\n", cmd->outfile, strerror(errno)); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        execvp(cmd->argv[0], cmd->argv);
        fprintf(stderr, "exec failed: %s: %s\n", cmd->argv[0], strerror(errno));
        exit(127);
    } else {
        /* parent */
        if (cmd->background) {
            /* background: don't wait */
            printf("[bg] pid %d\n", pid);
            return 0;
        } else {
            int status;
            waitpid(pid, &status, 0);
            return status;
        }
    }
}

/* Execute a pipeline of two commands: left | right */
static int execute_pipe(Command *left, Command *right) {
    int pipefd[2];
    if (pipe(pipefd) < 0) { perror_continue("pipe"); return -1; }

    pid_t p1 = fork();
    if (p1 < 0) { perror_continue("fork"); return -1; }
    if (p1 == 0) {
        /* left child */
        signal(SIGINT, SIG_DFL);
        /* if left has outfile? that's unusual, but allow it (stdout may be redirected to file, then to pipe? we favor pipe) */
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]); close(pipefd[1]);
        if (left->infile) {
            int fd = open(left->infile, O_RDONLY);
            if (fd < 0) { fprintf(stderr,"Failed to open %s: %s\n", left->infile, strerror(errno)); exit(1); }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        execvp(left->argv[0], left->argv);
        fprintf(stderr, "exec left failed: %s: %s\n", left->argv[0], strerror(errno));
        exit(127);
    }

    pid_t p2 = fork();
    if (p2 < 0) { perror_continue("fork"); return -1; }
    if (p2 == 0) {
        /* right child */
        signal(SIGINT, SIG_DFL);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]); close(pipefd[1]);
        if (right->outfile) {
            int fd = open(right->outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd < 0) { fprintf(stderr,"Failed to open %s: %s\n", right->outfile, strerror(errno)); exit(1); }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        execvp(right->argv[0], right->argv);
        fprintf(stderr, "exec right failed: %s: %s\n", right->argv[0], strerror(errno));
        exit(127);
    }

    /* parent */
    close(pipefd[0]); close(pipefd[1]);
    /* If background flag set on right (or left), treat whole pipeline as background.
       For simplicity, we check right->background first, then left->background */
    if (right->background || left->background) {
        printf("[bg] pids %d %d\n", p1, p2);
    } else {
        int status;
        waitpid(p1, &status, 0);
        waitpid(p2, &status, 0);
    }
    return 0;
}

int main(void) {
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    /* Parent should ignore SIGINT so Ctrl-C doesn't exit the shell */
    struct sigaction sa;
    sa.sa_handler = sigint_handler_parent;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    while (1) {
        /* Print prompt */
        if (isatty(STDIN_FILENO)) {
            fputs(PROMPT, stdout);
            fflush(stdout);
        }

        linelen = getline(&line, &linecap, stdin);
        if (linelen == -1) {
            /* EOF (Ctrl-D) or error */
            if (feof(stdin)) {
                printf("\n");
                break;
            } else {
                perror_continue("getline");
                continue;
            }
        }

        /* Remove trailing newline and trim */
        if (linelen > 0 && line[linelen-1] == '\n') line[linelen-1] = '\0';
        char *cmdline = trim(line);
        if (cmdline[0] == '\0') continue;

        /* Tokenize */
        char *tokens[MAX_TOKENS];
        int ntok = tokenize(cmdline, tokens, MAX_TOKENS);
        if (ntok == 0) continue;

        /* Detect pipe token '|' (only single pipe supported) */
        int pipe_index = -1;
        for (int i = 0; i < ntok; ++i) {
            if (strcmp(tokens[i], "|") == 0) { pipe_index = i; break; }
        }

        if (pipe_index >= 0) {
            /* parse left (0..pipe_index-1) and right (pipe_index+1..ntok-1) */
            Command left, right;
            if (pipe_index == 0 || pipe_index == ntok-1) {
                fprintf(stderr, "syntax error: misplaced pipe\n");
                free_tokens(tokens, ntok);
                continue;
            }
            if (parse_command_from_tokens(tokens, 0, pipe_index, &left) < 0) {
                free_tokens(tokens, ntok);
                continue;
            }
            if (parse_command_from_tokens(tokens, pipe_index+1, ntok, &right) < 0) {
                free_command(&left);
                free_tokens(tokens, ntok);
                continue;
            }
            /* If either side is builtin, we still attempt exec normally (not supported in pipeline) */
            execute_pipe(&left, &right);
            free_command(&left);
            free_command(&right);
        } else {
            Command cmd;
            if (parse_command_from_tokens(tokens, 0, ntok, &cmd) < 0) {
                free_tokens(tokens, ntok);
                continue;
            }
            execute_single(&cmd);
            free_command(&cmd);
        }

        free_tokens(tokens, ntok);
    }

    free(line);
    return 0;
}