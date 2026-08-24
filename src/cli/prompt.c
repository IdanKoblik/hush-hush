#include "cli/prompt.h"

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int read_passphrase(const char *prompt, char *out, size_t size) {
    struct termios original;
    int silent = isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &original) == 0;

    if (silent) {
        struct termios quiet = original;
        quiet.c_lflag &= ~(tcflag_t)ECHO;
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &quiet) != 0)
            silent = 0;
    }

    printf("%s", prompt);
    fflush(stdout);

    char *line = fgets(out, (int)size, stdin);

    if (silent) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
        printf("\n");
    }

    if (!line) {
        out[0] = '\0';
        return feof(stdin) ? 0 : -1;
    }

    out[strcspn(out, "\r\n")] = '\0';

    return 0;
}
