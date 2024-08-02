#include <stdio.h>
#include <string.h>

#define WORD_MAX 512
#define FUNCTION_BODY " { return 0; }"

#define _STR(x) #x
#define STR(x) _STR(x)

typedef struct {
    int num_braces; /* in { ... } */
    unsigned int open_parenthesis:1; /* check for '(' */
    unsigned int closed_parenthesis:1; /* check for ')' */
    unsigned int star:1; /* check for '*' */
    unsigned int __cplusplus:1; /* check for extern "C" { */
    unsigned int define_function:1; /* make sure we don't have a function pointer */
} state_t;

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s [input] [output]\n", argv[0]);
        return 0;
    }

    FILE *f = fopen(argv[1], "r");
    FILE *g = fopen(argv[2], "w");

    state_t state = (state_t){0};

    for (;;) {
        char str[WORD_MAX + sizeof(FUNCTION_BODY) + 1];
        if (fscanf(f, "%" STR(WORD_MAX) "s", str) == EOF) {
            return 0;
        }
        char c;
        if (fscanf(f, "%c", &c) == EOF) { /* file is not '\n' terminated */
            c = '\n';
        }

        if (!strncmp(str, "//", sizeof("//") - 1)) { /* ignore line comments */
            while (c != '\n') {
                if(fscanf(f, "%c", &c) == EOF) {
                    c = '\n';
                }
            }
            fprintf(g, "\n");
            continue;
        }

        if (!strncmp(str, "/\*", sizeof("/\*") - 1)) { /* ignore comments */
            while (c != '*') {
                if(fscanf(f, "%c", &c) == EOF) {
                    return 0;
                }

                if (c != '*') {
                    continue;
                }
                if((fscanf(f, "%c", &c) == EOF) || (c == '/')) {
                    c = ' '; /* for print_char */
                    break;
                }
            }
            fprintf(g, " ");
            goto print_char;
        }

        if (!strcmp(str, "__cplusplus")) {
            state.__cplusplus = 1;
            goto print;
        }

        if (str[0] == '#') {
            fprintf(g, "%s%c", str, c);
            while (c != '\n') {
                if(fscanf(f, "%c", &c) == EOF) {
                    c = '\n';
                }
                fprintf(g, "%c", c);
            }
            continue;
        }

        if (!strcmp(str, "typedef")) {
            fprintf(g, "%s%c", str, c);
            while (c != ';') {
                char eof = 0;
                if(fscanf(f, "%c", &c) == EOF) {
                    c = ';';
                    eof = 1;
                }
                fprintf(g, "%c", c);
                if (eof) {
                    fprintf(g, "\n");
                }
            }
            continue;
        }

        for (char *p = str; *p; p++) {
            if (*p == '{') {
                if (!state.__cplusplus) {
                    state.num_braces++;
                }
                else {
                    state.__cplusplus = 0;
                }
            }

            if (*p == '}') {
                if (!state.__cplusplus) {
                    state.num_braces--;
                }
                else {
                    state.__cplusplus = 0;
                }
            }
        }

        if (state.num_braces) {
            state.open_parenthesis = 0;
            state.closed_parenthesis = 0;
            state.star = 0;
            goto print;
        }

        if (!state.open_parenthesis && strchr(str, '(')) {
            state.open_parenthesis = 1;
        }

        if (state.open_parenthesis && !state.closed_parenthesis && strchr(str, ')')) {
            state.closed_parenthesis = 1;
        }

        if (state.open_parenthesis && !state.star && strchr(str, '*')) {
            state.star = 1;
        }

        state.define_function = state.closed_parenthesis && (!state.star || (strchr(str, ')') < strchr(str, '*')));

        if (state.define_function) { /* not a function pointer */
            char *test_char = strrchr(str, ';');
            if (test_char) {
                strcpy(test_char, FUNCTION_BODY);
                state.define_function = 0;
                state.open_parenthesis = 0;
                state.closed_parenthesis = 0;
                state.star = 0;
            }
        }

        if (!strcmp(str, "extern")) { /* define externs */
            continue;
        }
print:
        fprintf(g, "%s%c", str, c);
        if (c == '\n') {
            continue;
        }
print_char:
        while (c == ' ') { /* remove extra spaces */
            if (fscanf(f, "%c", &c) == EOF) { /* missing '\n' terminator */
                break;
            }
        }
        if (c == '\n') {
            fprintf(g, "\n");
            continue;
        }

        if (ungetc(c, f) == EOF) {
            str[0] = c;
        }
    }

    /* never reached */
    return 0;
}
