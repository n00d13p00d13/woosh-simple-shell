#include "vector.h"
#include <ctype.h>
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void die(const char *error, int errnum);
void display_prompt();
void get_line(FILE* input, Vector* vector);
char* vector_tokenize(Vector* input);
int parse_input(Vector* output, const Vector* input);


int main(int argc, char *argv[]) {
    Vector* history = vector_create(sizeof(Vector*), 64);

    int i = 0;
    while (i < 5) {
        Vector* buf = vector_create(sizeof(char), 256);
        Vector* parsed = vector_create(sizeof(char*), 64);

        get_line(stdin, buf);

        if (parse_input(parsed, buf) != 0) { die("parse_input", 1); }
        for (size_t i = 0; i < vector_size(parsed) - 1; i++) {
            char* token = *(char**)vector_get(parsed, i);
            printf("\"%s\"", token);
        }
        printf("\n");


        vector_push_back(history, &buf);
        vector_free(parsed);
        i++;
    }

    for (int i = 0; i < vector_size(history); i++) {
        Vector* buf = *(Vector**)vector_get(history, i);
        printf("%s\n", (char*)vector_arr(buf));
    }
    // poopoo

    return 0;
}


void die(const char *error, int errnum) {
    perror(error);
    exit(errnum);
}

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

        printf("\n\n\n[%s@%s %s]\n >>> ", user, hostname, &(cwd[trunc_cwd + 1]));
    }
}

void get_line(FILE* input, Vector* vector) {
    display_prompt();
    char c = 0;

    while (c != '\n' && c != EOF) {
        c = fgetc(input);
        vector_push_back(vector, &c);
    }
    c = '\0';
    vector_pop_back(vector, NULL);
    vector_push_back(vector, &c);
    return;
}


int parse_input(Vector* output, const Vector* input) {
    char* input_str = strdup((char*)vector_arr(input));
    char* token;
    token = strtok(input_str, " ");
    if (token == NULL) { return -1; }
    while (token != NULL) {
        vector_push_back(output, &token);
        token = strtok(NULL, " ");
    }
    char* null_ptr = NULL;
    vector_push_back(output, &null_ptr);
    return 0;
}

