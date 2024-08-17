#include <stdio.h>

typedef struct {
    unsigned int string:1;
    unsigned int escaped:1;
    unsigned int character:1;
} state_t;

int main(int argc, char **argv)
{
    FILE *f, *g;

    switch (argc) {
    case 1:
        f = stdin;
        g = stdout;
        break;
    case 2:
        f = fopen(argv[1], "r");
        g = stdout;
        break;
    case 3:
        f = fopen(argv[1], "r");
        g = fopen(argv[2], "w");
        break;
    default:
        fprintf(stderr, "USAGE: %s [input] [output]", argv[0]);
        return 0;
    }
    state_t state = (state_t){0};
    for (;;) {
        char c;
        if (fscanf(f, "%c", &c) == EOF) {
            return 0;
        }

        if (c == '"' && !state.escaped && !state.character) {
            state.string ^= 1;
        }

        if (c == '\'' && !state.escaped) {
            state.character ^= 1;
        }

        if (c == '\\') {
            state.escaped ^= 1;
        }
        else {
            state.escaped = 0;
        }


        if (c != '/') {
            goto print_char;
        }
        if (state.string) {
            goto print_char;
        }
        if (fscanf(f, "%c", &c) == EOF) { /* file not '\n' terminated */
            fprintf(g, "/\n");
            return 0;
        }
        if (c == '/') {
            for (;;) {
                if (fscanf(f, "%c", &c) == EOF) { /* file not '\n' terminated */
                    c = '\n';
                }
                if (c == '\n') {
                    goto print_char;
                }
            }
            /* never reached */
            continue;
        }
        if (c == '*') {
            for (;;) {
                if (fscanf(f, "%c", &c) == EOF) { /* unterminated comment */
                    fprintf(g, "\n");
                    return 0;
                }
                if (c != '*') {
                    continue;
                }
                if (fscanf(f, "%c", &c) == EOF) { /* unterminated comment */
                    fprintf(g, "\n");
                    return 0;
                }
                if (c == '/') {
                    break;
                }
            }
            continue;
        }
        fprintf(g, "/");
print_char:
        fprintf(g, "%c", c);
    }
}
