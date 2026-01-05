#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


#define MAX 1024

char last[MAX]= "";



/* ---------- Parse input ---------- */
void parse(char *line, char **args) {
    int i = 0;
    char *token = strtok(line, " ");
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;
}

int main() {
    char line[MAX];
    char *args[MAX];
    int should_run = 1; 

    while (should_run) {
        /* reap zombie processes */
        while (waitpid(-1, NULL, WNOHANG) > 0);

        printf("sh> ");
        if (!fgets(line, MAX, stdin))
            break;

        line[strcspn(line, "\n")] = 0;

        // history
        if (!strcmp(line, "!!")) {
            if (!strlen(last)) {
                printf("No history\n");
                continue;
            }
            strcpy(line, last);
            printf("%s\n", line);
        } else
            strcpy(last, line);

        /* ---------- parse ---------- */
        parse(line, args);
        if (args[0] == NULL)
            continue;

        //exit
        if (!strcmp(args[0], "exit")) {
            should_run = 0;   // 🔹 terminate shell
            continue;
        }
        //pwd
        if (!strcmp(args[0] ,"pwd")){
            char cwd[MAX];
            getcwd(cwd,sizeof(cwd));
            printf("%s\n",cwd);
            continue;
        
        }

        //cd
        if(strcmp(args[0], "cd")==0){
            if (chdir (args[1] )!= 0){
                perror("cd failed");
            }
            continue;
        }
        // help
        if (strcmp(args[0], "help") == 0) {
            printf("Available commands:\n");
            printf("  exit        - terminate the shell\n");
            printf("  cd <dir>    - change directory\n");
            printf("  pwd         - print working directory\n");
            printf("  help        - show this help message\n");
            printf("  !!          - repeat last command\n");
            printf("  cmd &       - run command in background\n");
            printf("  cmd1 | cmd2 - pipe output to another command\n");
            continue;
        }

        //background(&)
        int background = 0;
        int i = 0;
        while (args[i] != NULL) i++;

        if (i > 0 && !strcmp(args[i - 1], "&")) {
            background = 1;
            args[i - 1] = NULL;
        }


        /* ---------- pipe (|) ---------- */
        int pipe_index = -1;
        for (i = 0; args[i] != NULL; i++) {
            if (strcmp(args[i], "|") == 0) {
                pipe_index = i;
                break;
            }
        }

        if (pipe_index != -1) {
            args[pipe_index] = NULL;
            char **cmd1 = args;
            char **cmd2 = &args[pipe_index + 1];

            int fd[2];
            if (pipe(fd) < 0) {
                perror("pipe failed");
                continue;
            }

            pid_t p1 = fork();
            if (p1 < 0) {
                perror("Fork failed");
                continue;
            }

            if (p1 == 0) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
                if (execvp(cmd1[0], cmd1) == -1) {
                    printf("Command not found: %s\n", cmd1[0]);
                    exit(1);
                }
            }

            pid_t p2 = fork();
            if (p2 < 0) {
                perror("Fork failed");
                continue;
            }

            if (p2 == 0) {
                dup2(fd[0], STDIN_FILENO);
                close(fd[1]);
                close(fd[0]);
                if (execvp(cmd2[0], cmd2) == -1) {
                    printf("Command not found: %s\n", cmd2[0]);
                    exit(1);
                }
            }

            close(fd[0]);
            close(fd[1]);
            wait(NULL);
            wait(NULL);
            continue;
        }
        


         pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
            continue;
        }

        if (pid == 0) {
            if (execvp(args[0], args) == -1) {
                printf("Command not found: %s\n", args[0]);
                exit(1);

            }
        } else {
            if (!background)
                wait(NULL);
            else
                printf("[Running in background]\n");
        }
    }

    return 0;
        




            
}