#include "vector.h"
#include <ctype.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
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

// shell functions
void display_prompt();
void handle_redirection(Vector* args);
void execute(Vector* args, int is_background);
int chk_bgd(Vector* args);
int chk_internal(Vector* args, Vector* hist);

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

    while (1) {
        Vector* buf = vector_create(sizeof(char), 256);
        Vector* parsed = vector_create(sizeof(char*), 64);

        get_line(stdin, buf);
        vector_push_back(history, &buf);

        char* input_str = parse_input(parsed, buf);

        if (input_str == NULL) { 
            vector_free(parsed);
            vector_free(buf);
            vector_pop_back(history, NULL);
            continue; 
        }
        //        print_tokens(parsed);

        if (chk_internal(parsed, history) != 0) {
            free(input_str);
            vector_free(parsed);
            continue;
        }

        int background = chk_bgd(parsed);
        execute(parsed, background);

        free(input_str);
        vector_free(parsed);
    }

    for (size_t i = 0; i < vector_size(history); i++) {
        Vector* v = *(Vector**)vector_get(history, i);
        vector_free(v);
    }
    vector_free(history);
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
    char* token;
    token = strtok(input_str, " "); // ownership is with input_str
    if (token == NULL) { 
        free(input_str);
        return NULL; 
    }

    while (token != NULL) {
        vector_push_back(output, &token);
        token = strtok(NULL, " ");
    }

    char* null_ptr = NULL;
    vector_push_back(output, &null_ptr);
    return input_str;
}

void handle_signal(int sig) {
    if (sig == SIGINT) {
        display_prompt();
        fflush(stdout);
    }
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

        printf("\n\n[%s@%s %s]\n >>> ", user, hostname, &(cwd[trunc_cwd + 1]));
    }
}

void handle_redirection(Vector* args) {
    for (int i = 0; vector_get_string(args, i) != NULL && vector_size(args) > 0; i++) {
        if (strcmp(vector_get_string(args, i), ">") == 0) {
            int fd = open(vector_get_string(args, i+1), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { die("handle_redirection", 1); }
            dup2(fd, STDOUT_FILENO);    // redirect fd to stdout so all output is redirected to fd
            close(fd);
            char* null_ptr = NULL;
            vector_set(args, &null_ptr, i);
        }
        else if (strcmp(vector_get_string(args, i), ">>") == 0) {
            int fd = open(vector_get_string(args, i+1), O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (fd < 0) { die("handle_redirection", 1); }
            dup2(fd, STDOUT_FILENO);    // redirect fd to stdout so all output is redirected to fd
            close(fd);
            char* null_ptr = NULL;
            vector_set(args, &null_ptr, i);
        }
        else if (strcmp(vector_get_string(args, i), "<") == 0) {
            int fd = open(vector_get_string(args, i+1), O_RDONLY);
            if (fd < 0) { die("handle_redirection", 1); }
            dup2(fd, STDIN_FILENO); // redirect fd to stdin to redirect any output from the file
            close(fd);
            char* null_ptr = NULL;
            vector_set(args, &null_ptr, i);
        
        }
    }
}

void execute(Vector* args, int is_background) {
    int status;

    pid_t pid = fork();
    if (pid == 0) {
        // child
        handle_redirection(args);
        if (execvp(vector_get_string(args, 0), vector_arr(args)) == -1) {
            die("woosh", 1);
        }
    }

    else if (pid < 0) {
        print_err("couldnt execute command");
    }

    else {
        if (!is_background) {
            // parent
            waitpid(pid, &status, 0);
        }
        else {
            printf("process running in background with PID %d\n", pid);
            fflush(stdout);
        }
    }
}

int chk_bgd(Vector* args) {
    if (vector_size(args) < 2) return 0;

    if (strcmp(vector_get_string(args, vector_size(args) - 2), "&") == 0) {
        char* null_ptr = NULL;
        vector_set(args, &null_ptr, vector_size(args) - 2);
        vector_pop_back(args, NULL);
        return 1;
    }
    return 0;
}

int chk_internal(Vector* args, Vector* hist) {
    char* cmd = vector_get_string(args, 0);
    if (cmd == NULL) { return 0; } 

    if (strcmp(cmd, "exit") == 0) {
        exit(0);
    }
    else if (strcmp(cmd, "cd") == 0) {
        cd(args);
        return CD;
    }
    else if (strcmp(cmd, "help") == 0) {
        printf("this is a simple shell in C\nuse with caution\n");
        printf("internal commands include:\ncd, pwd, history\n");
        return CD;
    }
    else if (strcmp(cmd, "pwd") == 0) {
        pwd();
        return CD;
    }
    else if (strcmp(cmd, "history") == 0) {
        print_hist(hist);
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
