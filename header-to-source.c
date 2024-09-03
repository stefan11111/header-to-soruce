#include <stdio.h>
#include <string.h>

#define WORD_MAX 512
#define FUNCTION_BODY " { return 0; }"
#define VOID_FUNCTION_BODY " {}"

#define _STR(x) #x
#define STR(x) _STR(x)

typedef struct {
    int num_braces; /* in { ... } */
    int open_parenthesis; /* check for '(' */
    int star; /* check for '*' */
    unsigned int is_function_pointer:2; /* check whether between the first '(' and the first '*' */
                                        /* there are characters other than ' ' and '\n' */
    unsigned int is_void:1; /* check if the function type contains void */
    unsigned int is_not_void_function:1; /* check if the function is not void* */
    unsigned int closed_parenthesis:1; /* check for ')' */
    unsigned int define_function:1; /* make sure we don't have a function pointer */
    unsigned int __cplusplus:1; /* check for extern "C" { */
} state_t;

#define reset_parenthesis() \
    state.open_parenthesis = 0; \
    state.star = 0; \
    state.is_function_pointer = 0; \
    state.is_void = 0; \
    state.is_not_void_function = 0; \
    state.closed_parenthesis = 0; \
    state.define_function = 0; \

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
            reset_parenthesis();
            continue;
        }

        if (!strncmp(str, "/" "*", sizeof("/" "*") - 1)) { /* ignore comments */
            char *ptr = strstr(str, "*/");
            if (ptr) {
                char *dst = str;
                char *src = ptr + sizeof("*/") - 1;
                while ((*dst++ = *src++)); /* -Wparentheses */
                goto past_comments;
            }
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
past_comments:
        if (str[0] == '#') {
            fprintf(g, "%s%c", str, c);
            char *cpp = "__cplusplus";
            unsigned char ind = 0;
            while (c != '\n') {
                if(fscanf(f, "%c", &c) == EOF) {
                    c = '\n';
                }

                if(cpp[ind] == c) {
                    ind++;
                }
                else {
                    ind = 0;
                }

                if(cpp[ind] == '\0') {
                    state.__cplusplus = 1;
                }

                fprintf(g, "%c", c);
            }
            reset_parenthesis();
            continue;
        }

        if (!strcmp(str, "typedef")) {
            fprintf(g, "%s%c", str, c);
            while (c != ';') {
                if(fscanf(f, "%c", &c) == EOF) {
                    c = ';';
                }
                fprintf(g, "%c", c);
            }

            while (c != '\n') {
                if(fscanf(f, "%c", &c) == EOF) {
                    c = '\n';
                }
                fprintf(g, "%c", c);
            }
            reset_parenthesis();
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
            reset_parenthesis();
            goto print;
        }

        for (char *p = strchr(str, '('); p; p = strchr(p + 1, '(')) {
            state.open_parenthesis++;
        }

        if (state.open_parenthesis && !state.star && state.is_function_pointer != 2) {
            state.is_function_pointer = 1; /* mark that it needs to be checked */
        }

        if (state.open_parenthesis && !state.star && state.is_function_pointer == 1) {
            char *q = strchr(str, '('); /* if there are more that 1 '(' , the result of this doesn't matter */
            for (char *p = q ? q + 1 : str ; *p && *p != '*'; p++) {
                if (*p != ' ' && *p != '\n') {
                    state.is_function_pointer = 2; /* checked and is not function pointer */
                    break;
                }
            }
        }

        if (state.open_parenthesis && strchr(str, ')')) {
            state.closed_parenthesis = 1;
        }

        if (state.open_parenthesis && !state.star && strchr(str, '*')) {
            state.star = state.open_parenthesis;
        }

        if (!state.open_parenthesis && strstr(str, "void")) {
            state.is_void = 1;
        }

        if (state.is_void && !state.open_parenthesis && strchr(str, '*')) {
            state.is_not_void_function = 1;
        }

        state.define_function = ((state.open_parenthesis == 1) ||
                                 (state.closed_parenthesis && state.star != 1) ||
                                 (state.is_function_pointer == 2));

        char *test_char = strrchr(str, ';');

        if (state.define_function && test_char) { /* not a function pointer */

            (!state.is_not_void_function && state.is_void) ? strcpy(test_char, VOID_FUNCTION_BODY) :
                                                             strcpy(test_char, FUNCTION_BODY);
            reset_parenthesis();
        }

        if (!state.define_function && test_char) { /* function pointer */
            reset_parenthesis();
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
