#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>

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
long long total_inside = 0;
pthread_mutex_t mc_mutex;
int sudoku[9][9];
int valid[11]; 
typedef struct {
    int row;
    int column;
    int index;
} parameters;

typedef struct {
    long long points;
    unsigned int seed;
} mc_args;


void *check_rows(void *param) {
    int seen[10];

    for (int i = 0; i < 9; i++) {
        memset(seen, 0, sizeof(seen));
        for (int j = 0; j < 9; j++) {
            int num = sudoku[i][j];
            if (num < 1 || num > 9 || seen[num]) {
                valid[0] = 0;
                pthread_exit(NULL);
            }
            seen[num] = 1;
        }
    }
    valid[0] = 1;
    pthread_exit(NULL);
}

void *check_cols(void *param) {
    int seen[10];

    for (int j = 0; j < 9; j++) {
        memset(seen, 0, sizeof(seen));
        for (int i = 0; i < 9; i++) {
            int num = sudoku[i][j];
            if (num < 1 || num > 9 || seen[num]) {
                valid[1] = 0;
                pthread_exit(NULL);
            }
            seen[num] = 1;
        }
    }
    valid[1] = 1;
    pthread_exit(NULL);
}


void *check_subgrid(void *param) {
    parameters *p = (parameters *)param;
    int seen[10] = {0};

    for (int i = p->row; i < p->row + 3; i++) {
        for (int j = p->column; j < p->column + 3; j++) {
            int num = sudoku[i][j];
            if (num < 1 || num > 9 || seen[num]) {
                valid[p->index] = 0;
                pthread_exit(NULL);
            }
            seen[num] = 1;
        }
    }

    valid[p->index] = 1;
    pthread_exit(NULL);
}



void *monte_carlo_worker(void *arg) {
    mc_args *data = (mc_args *)arg;
    long long inside = 0;

    for (long long i = 0; i < data->points; i++) {
        double x = (double)rand_r(&data->seed) / RAND_MAX * 2.0 - 1.0;
        double y = (double)rand_r(&data->seed) / RAND_MAX * 2.0 - 1.0;

        if (x * x + y * y <= 1.0)
            inside++;
    }

    pthread_mutex_lock(&mc_mutex);
    total_inside += inside;
    pthread_mutex_unlock(&mc_mutex);

    pthread_exit(NULL);
}


int main() {
    setenv("PATH", "/usr/local/bin:/usr/bin:/bin", 1);
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


        if (strcmp(args[0], "sudoku") == 0) {
            if (args[1] == NULL) {
                printf("Usage: sudoku <file>\n");
                continue;
            }

            // ---------- File open ----------
            int fd = open(args[1], O_RDONLY);
            if (fd < 0) {
                perror("File open error");
                continue; //We use continue instead of exit(1) to prevent shell termination.
            }

            FILE *fp = fdopen(fd, "r");
            if (!fp) {
                perror("fdopen failed");
                close(fd);
                continue;
            }

            int invalid = 0;
            for (int i = 0; i < 9 && !invalid; i++) {
                for (int j = 0; j < 9; j++) {
                    if (fscanf(fp, "%d", &sudoku[i][j]) != 1) {
                        invalid = 1;
                        break;
                    }
                }
            }
            fclose(fp);
            if (invalid) {     
                printf("Invalid sudoku file\n");
                continue;
            }

            for (int i = 0; i < 11; i++)   
                valid[i] = 0;


            // ---------- Threads ----------
            pthread_t threads[11];
            parameters params[9];

            pthread_create(&threads[0], NULL, check_rows, NULL);
            pthread_create(&threads[1], NULL, check_cols, NULL);

            int idx = 2;
            for (int i = 0; i < 9; i += 3) {
                for (int j = 0; j < 9; j += 3) {
                    params[idx - 2].row = i;
                    params[idx - 2].column = j;
                    params[idx - 2].index = idx;
                    pthread_create(&threads[idx], NULL, check_subgrid, &params[idx - 2]);
                    idx++;
                }
            }

            for (int i = 0; i < 11; i++)
                pthread_join(threads[i], NULL);

            int ok = 1;
            for (int i = 0; i < 11; i++) {
                if (valid[i] == 0) {
                    ok = 0;
                    break;
                }
            }

            if (ok)
                printf("Sudoku is VALID ✅\n");
            else
                printf("Sudoku is INVALID ❌\n");

            continue;
        }





        // Monte Carlo Pi Estimation
        if (strcmp(args[0], "mont_carlo") == 0) {
            if (args[1] == NULL || args[2] == NULL) {
                printf("Usage: mont_carlo <threads> <points>\n");
                continue;
            }

            int threads_count = atoi(args[1]);
            long long total_points = atoll(args[2]);

            if (threads_count <= 0 || total_points <= 0) {
                printf("Invalid arguments\n");
                continue;
            }

            pthread_t threads[threads_count];
            mc_args params[threads_count];

            pthread_mutex_init(&mc_mutex, NULL);
            total_inside = 0;

            long long points_per_thread = total_points / threads_count;

            for (int i = 0; i < threads_count; i++) {
                params[i].points = points_per_thread;
                params[i].seed = time(NULL) ^ (i << 16);
                pthread_create(&threads[i], NULL, monte_carlo_worker, &params[i]);
            }

            for (int i = 0; i < threads_count; i++) {
                pthread_join(threads[i], NULL);
            }

            double pi = 4.0 * total_inside / total_points;
            printf("Estimated Pi = %.6f\n", pi);

            pthread_mutex_destroy(&mc_mutex);
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
//Parallel Sudoku Validator
//Monte Carlo Pi Estimation