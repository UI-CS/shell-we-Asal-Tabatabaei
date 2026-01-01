#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>


#define MAX 1024

char last[MAX];
int main() {
    char line[MAX];

    while (1) {
        printf("sh> ");
        fgets(line, MAX, stdin);
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
            strcpy(last, line);}
            
