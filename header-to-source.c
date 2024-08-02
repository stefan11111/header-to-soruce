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
    unsigned int is_typedef:1; /* check for typedef */
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
                    c = '*'; /* to exit loop */
                }
            }
            continue;
        }

        if (!strcmp(str, "__cplusplus")) {
            state.__cplusplus = 1;
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
            fprintf(g, "%s%c", str, c);
            state.open_parenthesis = 0;
            state.closed_parenthesis = 0;
            state.star = 0;
            continue;
        }

        if (!state.open_parenthesis && strchr(str, '(')) {
            state.open_parenthesis = 1;
        }

        if (!state.closed_parenthesis && strchr(str, ')')) {
            state.closed_parenthesis = 1;
        }

        if (!state.star && strchr(str, '*')) {
            state.star = 1;
        }

        state.define_function = state.closed_parenthesis && (!state.star || (strchr(str, ')') < strchr(str, '*')));

        if (state.define_function) { /* not a function pointer */
            char *test_char = strrchr(str, ';');
            if (test_char) {
                strcpy(test_char, FUNCTION_BODY);
                state.define_function = 0;
            }
        }

        if (!strcmp(str, "typedef")) {
            state.is_typedef = 1;
            continue;
        }
        state.is_typedef = 0;

        if (!state.is_typedef && !strcmp(str, "extern")) { /* define externs */
            continue;
        }

        fprintf(g, "%s%c", str, c);
    }

    /* never reached */
    return 0;
}
