#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  
#include <sys/types.h>
#include <sys/wait.h> 
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_CMD_LEN 1024
#define MAX_ARGS 64
void sigint_handler(int sig) {
    (void)sig;
    printf("\nmyshell> ");
    fflush(stdout);
}

void sigchld_handler(int sig) {
    (void)sig; 
    while (waitpid(-1,NULL,WNOHANG) > 0);
}
#define MAX_TOKENS 256
#define MAX_ARGV 128
#define PROMPT "myshell> "

typedef struct {
    char *argv[MAX_ARGV];
    char *infile;
    char *outfile;
    int background;
} Command;

static void perror_continue(const char *msg) {
    perror(msg);
}

static char *trim_sb(char *s) {
    if (!s) return s;
    while (*s && (*s == ' ' || *s == '\t' || *s == '\n')) s++;
    if (*s == '\0') return s;
    char *end=s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' || *end == '\n')) {
        *end='\0'; end--;
    }
    return s;
}
static int tokenize_sb(const char *line,char **tokens,int max_tokens) {
    const char *p=line;
    int ntok=0;

    while (*p && ntok<max_tokens) {
        while (*p == ' ' || *p == '\t' || *p == '\n') p++;
        if (!*p) break;
        if (*p == '"' || *p == '\''){
            char quote=*p++;
            const char *start =p;
            while (*p && *p != quote){
                if (*p == '\\' && quote == '"' && p[1]) p++;
                p++;
            }
            size_t len=p - start;
            char *tok=malloc(len + 1);
            if (!tok) { fprintf(stderr,"malloc failed\n"); exit(1); }
            strncpy(tok,start,len);
            tok[len]='\0';
            tokens[ntok++]=tok;
            if (*p == quote) p++;
        } else {
            if (*p == '>' || *p == '<' || *p == '|' || *p == '&') {
                char *tok=malloc(2);
                if (!tok) { fprintf(stderr,"malloc failed\n"); exit(1); }
                tok[0]=*p;
                tok[1]='\0';
                tokens[ntok++]=tok;
                p++;
            } else {
                const char *start=p;
                while (*p && !(*p == ' ' || *p == '\t' || *p == '\n' ||
                              *p == '>' || *p == '<' || *p == '|' || *p == '&')) {
                    p++;
                }
                size_t len=p - start;
                char *tok=malloc(len + 1);
                if (!tok) { fprintf(stderr,"malloc failed\n"); exit(1); }
                strncpy(tok,start,len);
                tok[len]='\0';
                tokens[ntok++]=tok;
            }
        }
    }
    return ntok;
}
static void free_tokens_sb(char **tokens,int n) {
    for (int i=0; i < n; ++i) free(tokens[i]);
}
static int parse_command_from_tokens_sb(char **tokens,int start,int end,Command *cmd) {
    memset(cmd,0,sizeof(Command));
    int argi=0;
    for (int i=start; i < end; ++i) {
        char *t=tokens[i];
        if (strcmp(t,"<") == 0) {
            if (i+1 >= end) { fprintf(stderr,"syntax error: expected filename after '<'\n"); return -1; }
            cmd->infile=strdup(tokens[++i]);
        } else if (strcmp(t,">") == 0) {
            if (i+1 >= end) { fprintf(stderr,"syntax error: expected filename after '>'\n"); return -1; }
            cmd->outfile=strdup(tokens[++i]);
        } else if (strcmp(t,"&") == 0) {
            if (i != end-1) {
                fprintf(stderr,"syntax warning: '&' not at end — treating as background\n");
            }
            cmd->background=1;
        } else {
            if (argi >= MAX_ARGV-1) { fprintf(stderr,"too many args\n"); return -1; }
            cmd->argv[argi++]=strdup(t);
        }
    }
    cmd->argv[argi]=NULL;
    return 0;
}
static void free_command_sb(Command *c) {
    for (int i=0; c->argv[i]; ++i) free(c->argv[i]);
    if (c->infile) free(c->infile);
    if (c->outfile) free(c->outfile);
}
static int execute_single_sb(Command *cmd) {
    if (!cmd->argv[0]) return 0;
    if (strcmp(cmd->argv[0],"exit") == 0) {
        exit(0);
    }
    if (strcmp(cmd->argv[0],"cd") == 0) {
        const char *dir=cmd->argv[1] ? cmd->argv[1] : getenv("HOME");
        if (chdir(dir) != 0) perror_continue("cd");
        return 0;
    }

    pid_t pid=fork();
    if (pid < 0) { perror_continue("fork"); return -1; }
    if (pid == 0) {
        signal(SIGINT,SIG_DFL);
        if (cmd->infile) {
            int fd=open(cmd->infile,O_RDONLY);
            if (fd < 0) { fprintf(stderr,"Failed to open %s: %s\n",cmd->infile,strerror(errno)); exit(1); }
            dup2(fd,STDIN_FILENO);
            close(fd);
        }
        if (cmd->outfile) {
            int fd=open(cmd->outfile,O_WRONLY | O_CREAT | O_TRUNC,0666);
            if (fd < 0) { fprintf(stderr,"Failed to open %s: %s\n",cmd->outfile,strerror(errno)); exit(1); }
            dup2(fd,STDOUT_FILENO);
            close(fd);
        }

        execvp(cmd->argv[0],cmd->argv);
        fprintf(stderr,"exec failed: %s: %s\n",cmd->argv[0],strerror(errno));
        exit(127);
    } else {
        if (cmd->background) {
            printf("[bg] pid %d\n",pid);
            return 0;
        } else {
            int status;
            waitpid(pid,&status,0);
            return status;
        }
    }
}

static int execute_pipe_sb(Command *left,Command *right) {
    int pipefd[2];
    if (pipe(pipefd) < 0) { perror_continue("pipe"); return -1; }

    pid_t p1=fork();
    if (p1 < 0) { perror_continue("fork"); return -1; }
    if (p1 == 0) {
        signal(SIGINT,SIG_DFL);
        dup2(pipefd[1],STDOUT_FILENO);
        close(pipefd[0]); close(pipefd[1]);
        if (left->infile) {
            int fd=open(left->infile,O_RDONLY);
            if (fd < 0) { fprintf(stderr,"Failed to open %s: %s\n",left->infile,strerror(errno)); exit(1); }
            dup2(fd,STDIN_FILENO);
            close(fd);
        }
        execvp(left->argv[0],left->argv);
        fprintf(stderr,"exec left failed: %s: %s\n",left->argv[0],strerror(errno));
        exit(127);
    }

    pid_t p2=fork();
    if (p2 < 0) { perror_continue("fork"); return -1; }
    if (p2 == 0) {
        signal(SIGINT,SIG_DFL);
        dup2(pipefd[0],STDIN_FILENO);
        close(pipefd[0]); close(pipefd[1]);
        if (right->outfile) {
            int fd=open(right->outfile,O_WRONLY | O_CREAT | O_TRUNC,0666);
            if (fd < 0) { fprintf(stderr,"Failed to open %s: %s\n",right->outfile,strerror(errno)); exit(1); }
            dup2(fd,STDOUT_FILENO);
            close(fd);
        }
        execvp(right->argv[0],right->argv);
        fprintf(stderr,"exec right failed: %s: %s\n",right->argv[0],strerror(errno));
        exit(127);
    }

    close(pipefd[0]); close(pipefd[1]);

    if (right->background || left->background) {
        printf("[bg] pids %d %d\n",p1,p2);
    } else {
        int status;
        waitpid(p1,&status,0);
        waitpid(p2,&status,0);
    }
    return 0;
}
int main() {
    char input[MAX_CMD_LEN];
    signal(SIGINT,sigint_handler);
    signal(SIGCHLD,sigchld_handler); 

    while (1) {
        printf("myshell> ");
        if (fgets(input,MAX_CMD_LEN,stdin) == NULL) break;
        input[strcspn(input,"\n")]=0;
	 char *lineptr=trim_sb(input);
	 if (lineptr[0] == '\0') continue;
	 char *saveptr=NULL;
	 char *sub=strtok_r(lineptr,";",&saveptr);
	 while (sub) {
	     char *subtrim=trim_sb(sub);
	     if (subtrim[0] != '\0') {
	         char *tokens[MAX_TOKENS];
	         int ntok=tokenize_sb(subtrim,tokens,MAX_TOKENS);
	         if (ntok == 0) {
	             free_tokens_sb(tokens,ntok);
	             sub=strtok_r(NULL,";",&saveptr);
	             continue;
	         }
	         int pipe_index=-1;
	         for (int i=0; i < ntok; ++i) {
	             if (strcmp(tokens[i],"|") == 0) { pipe_index=i; break; }
	         }

	         if (pipe_index >= 0) {
	             Command left,right;
	             if (pipe_index == 0 || pipe_index == ntok - 1) {
	                 fprintf(stderr,"syntax error: misplaced pipe\n");
	                 free_tokens_sb(tokens,ntok);
	                 sub=strtok_r(NULL,";",&saveptr);
	                 continue;
	             }
	             if (parse_command_from_tokens_sb(tokens,0,pipe_index,&left) < 0) {
	                 free_tokens_sb(tokens,ntok);
	                 sub=strtok_r(NULL,";",&saveptr);
	                 continue;
	             }
	             if (parse_command_from_tokens_sb(tokens,pipe_index + 1,ntok,&right) < 0) {
	                 free_command_sb(&left);
	                 free_tokens_sb(tokens,ntok);
	                 sub=strtok_r(NULL,";",&saveptr);
	                 continue;
	             }
	             execute_pipe_sb(&left,&right);
	             free_command_sb(&left);
	             free_command_sb(&right);
	         } else {
	             Command cmd;
	             if (parse_command_from_tokens_sb(tokens,0,ntok,&cmd) < 0) {
	                 free_tokens_sb(tokens,ntok);
	                 sub=strtok_r(NULL,";",&saveptr);
	                 continue;
	             }
	             execute_single_sb(&cmd);
	             free_command_sb(&cmd);
	         }

	         free_tokens_sb(tokens,ntok);
	     }
	     sub=strtok_r(NULL,";",&saveptr);
	 }
    }
    return 0;
}
