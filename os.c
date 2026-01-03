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

        //exit
        if (!strcmp (line,"exit")) break; 
        //pwd
        if (!strcmp(line ,"pwd")){
            char cwd[1024];
            getcwd(cwd,sizeof(cwd));
            printf("%s\n",cwd);
            continue;
        
        }
        /* ---------- parse ---------- */
        parse(line, args);
        if (args[0] == NULL)
            continue;


            
}}