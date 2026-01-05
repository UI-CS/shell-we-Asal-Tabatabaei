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

    while (1) {
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



            
}}