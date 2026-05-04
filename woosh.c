#include "vector.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define CD 10
#define HIST 20
#define PWD 30

// helper functions
void die(const char *error, int errnum);
void print_err(const char* error);
void print_tokens(const Vector* vector);
void get_line(FILE* input, Vector* vector);
char* parse_input(Vector* output, const Vector* input);
void handle_signal(int sig);
void vector_free_wrapper(Vector* vector);
// shell functions
void display_prompt();
void handle_redirection(Vector* args);
void execute_pipeline(Vector* cmds, int is_background);
int chk_bgd(Vector* cmds);
int chk_internal(Vector* cmds, Vector* hist);
// internal commands
int cd(Vector* args);
void pwd();
void print_hist(const Vector* history);


int main(int argc, char *argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("this is a simple shell in C\nuse with caution\n");
            printf("internal commands include:\ncd, pwd, history\n");
        }
        else {
            printf("arguments not supported\n");
        }
        return 1;
    }

    Vector* history = vector_create(sizeof(Vector*), 64);

    signal(SIGINT, handle_signal);
    signal(SIGTSTP, handle_signal);

    while (1) {
        Vector* buf = vector_create(sizeof(char), 256);
        Vector* cmds = vector_create(sizeof(Vector*), 64);

        get_line(stdin, buf);
        vector_push_back(history, &buf);

        char* input_str = parse_input(cmds, buf);

        if (input_str == NULL) { 
            vector_free_wrapper(cmds);
            vector_free(buf);
            vector_pop_back(history, NULL);
            continue; 
        }

        if (chk_internal(cmds, history) != 0) {
            free(input_str);
            vector_free_wrapper(cmds);
            continue;
        }

        int background = chk_bgd(cmds);
        execute_pipeline(cmds, background);

        free(input_str);
        vector_free_wrapper(cmds);
    }

    vector_free_wrapper(history);
    return 0;
}


// HELPER FUNCTIONS
void die(const char *error, int errnum) {
    perror(error);
    exit(errnum);
}

void print_err(const char* error) {
    fprintf(stderr, "woosh: %s\n", error);
}

void print_tokens(const Vector* vector) {
    for (size_t i = 0; i < vector_size(vector) - 1; i++) {
        char* token = vector_get_string(vector, i);
        printf("\"%s\" ", token);
    }
    printf("\n");
}

void get_line(FILE* input, Vector* vector) {
    display_prompt();
    char c = 0;

    while (c != '\n' && c != EOF) {
        c = fgetc(input);
        vector_push_back(vector, &c);
    }
    c = '\0';
    vector_pop_back(vector, NULL);  // remove the annoying \n
    vector_push_back(vector, &c);
    return;
}

char* parse_input(Vector* output, const Vector* input) {
    char* input_str = strdup((char*)vector_arr(input));
    char* null_ptr = NULL;
    char* token;

    if (input_str == NULL) { return NULL; }
    token = strtok(input_str, " ");
    if (token == NULL) {
        free(input_str);
        return NULL;
    }

    Vector* current_cmd = vector_create(sizeof(char*), 24);

    while (token != NULL) {
        if (strcmp(token, "|") == 0) {
            vector_push_back(current_cmd, &null_ptr);
            vector_push_back(output, &current_cmd);

            current_cmd = vector_create(sizeof(char*), 24);
        } 
        else {
            vector_push_back(current_cmd, &token);
        }
        token = strtok(NULL, " ");
    }

    vector_push_back(current_cmd, &null_ptr);
    vector_push_back(output, &current_cmd);
    return input_str;
}

void handle_signal(int sig) {
    if (sig == SIGINT) {
        display_prompt();
        fflush(stdout);
    }
    else if (sig == SIGTSTP) {
        display_prompt();
        fflush(stdout);
    }
}

void vector_free_wrapper(Vector* vector) {
    for (size_t i = 0; i < vector_size(vector); i++) {
        Vector* v = *(Vector**)vector_get(vector, i);
        vector_free(v);
    }
    vector_free(vector);
}



// SHELL COMMANDS
void display_prompt() {
    char cwd[100];
    char* user = getlogin();
    char hostname[100];

    if (getcwd(cwd, 100) != NULL && gethostname(hostname, 100) == 0) {
        int trunc_cwd = 0;
        for (int i = 100-1; i > 0; i--) {
            if (cwd[i] == '/') {
                trunc_cwd = i;
                break;
            }
        }

        printf("\n[%s@%s %s]\n >>> ", user, hostname, &(cwd[trunc_cwd + 1]));
    }
}

void handle_redirection(Vector* args) {
    for (int i = 0; vector_get_string(args, i) != NULL && vector_size(args) > 0; i++) {
        int fd = -1;
        if (strcmp(vector_get_string(args, i), ">") == 0) {
            fd = open(vector_get_string(args, i+1), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { die("handle_redirection", 1); }
            dup2(fd, STDOUT_FILENO);    // redirect fd to stdout so all output is redirected to fd
        }
        else if (strcmp(vector_get_string(args, i), ">>") == 0) {
            fd = open(vector_get_string(args, i+1), O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) { die("handle_redirection", 1); }
            dup2(fd, STDOUT_FILENO);    // redirect fd to stdout so all output is redirected to fd
        }
        else if (strcmp(vector_get_string(args, i), "<") == 0) {
            fd = open(vector_get_string(args, i+1), O_RDONLY);
            if (fd < 0) { die("handle_redirection", 1); }
            dup2(fd, STDIN_FILENO); // redirect fd to stdin to redirect any output from the file
        }

        if (fd != -1) {
            close(fd);
            vector_remove(args, i); 
        }
    }
}

void execute_pipeline(Vector* cmds, int is_background) {
    int num_cmds = vector_size(cmds);
    int pipefds[2 * (num_cmds - 1)]; // 2 file descriptors per pipe
    pid_t pids[num_cmds];

    // create pipes for all cmds
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            die("pipe failed", 1);
        }
    }

    // fork and execute each cmd
    for (int i = 0; i < num_cmds; i++) {
        Vector* cmd_args = *(Vector**)vector_get(cmds, i);

        pids[i] = fork();
        if (pids[i] == 0) {
            // child process
            if (i > 0) {
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);
            }

            if (i < num_cmds - 1) {
                dup2(pipefds[(i * 2) + 1], STDOUT_FILENO);
            }
            
            for (int j = 0; j < 2 * (num_cmds - 1); j++) {
                close(pipefds[j]);  // close all pipefds in all children processes
            }

            handle_redirection(cmd_args);
            if (execvp(vector_get_string(cmd_args, 0), vector_arr(cmd_args)) == -1) {
                die("woosh: command execution failed", 1);
            }
        } 
        else if (pids[i] < 0) {
            print_err("couldn't fork for pipeline");
        }
    }

    // parent
    for (int i = 0; i < 2 * (num_cmds - 1); i++) {
        close(pipefds[i]);  // close all child fds in parent since its unnecessary
    }

    if (!is_background) {
        int status;
        for (int i = 0; i < num_cmds; i++) {
            waitpid(pids[i], &status, 0);
        }
    } 
    else {
        printf("pipeline running in background\n");
        fflush(stdout);
    }
}

int chk_bgd(Vector* cmds) {
    if (vector_size(cmds) < 2) return 0;

    Vector* last_cmd = *(Vector**)vector_get(cmds, vector_size(cmds) - 1);
    if (strcmp(vector_get_string(last_cmd, vector_size(cmds) - 2), "&") == 0) {
        char* null_ptr = NULL;
        vector_set(last_cmd, &null_ptr, vector_size(last_cmd) - 2);
        vector_pop_back(last_cmd, NULL);
        return 1;
    }
    return 0;
}

int chk_internal(Vector* cmds, Vector* hist) {
    char* cmd = vector_get_string(*(Vector**)vector_get(cmds, 0), 0);
    if (cmd == NULL) { return 0; } 

    if (strcmp(cmd, "exit") == 0) {
        exit(0);
    }
    else if (strcmp(cmd, "cd") == 0) {
        cd(cmds);
        return CD;
    }
    else if (strcmp(cmd, "clear") == 0) {
        // \e provides an escape. 
        // exp [1;1H] place your cursor in the upper right corner of the console screen. 
        // exp [2J adds a space to the top of all existing screen characters.
        printf("\e[1;1H\e[2J");
        return 1;
    }
    else if (strcmp(cmd, "help") == 0) {
        printf("this is a simple shell in C\nuse with caution\n");
        printf("internal commands include:\ncd, pwd, history\n");
        return 1;
    }
    else if (strcmp(cmd, "pwd") == 0) {
        pwd();
        return PWD;
    }
    else if (strcmp(cmd, "history") == 0) {
        print_hist(hist);
        Vector* v = *(Vector**)vector_get(hist, vector_size(hist) - 1);
        vector_free(v);
        char* null_ptr = NULL;
        vector_set(hist, &null_ptr, vector_size(hist) - 1);
        vector_pop_back(hist, NULL);
        return HIST;
    }
    return 0;
}


// INTERNAL COMMANDS
int cd(Vector* args) {
    if (args == NULL) { print_err("cd args is null"); }
    if (vector_get_string(args, 1) == NULL) { 
        print_err("expected directory for \"cd\"");
    }
    else {
        if (chdir(vector_get_string(args, 1)) != 0) {
            print_err("invalid directory for \"cd\"");
        }
    }
    return 1;
}

void pwd() {
    char cwd[100];
    getcwd(cwd, 100);
    printf("Current Working Directory: \n%s\n", cwd);
}

void print_hist(const Vector* history) {
    for (size_t i = 0; i < vector_size(history) - 1; i++) {
        Vector* buf = *(Vector**)vector_get(history, i);
        printf("%s\n", (char*)vector_arr(buf));
    }
    printf("\nhist size: %u\n", (unsigned int)vector_size(history) - 1);
}
