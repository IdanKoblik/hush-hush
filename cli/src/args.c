#include "args.h"
#include <stddef.h>

char *shift_args(int *argc, char **argv[]) {
    if (*argc <= 0)
        return NULL;

    char *arg = **argv;
    *argv += 1;
    *argc -= 1;
    return arg;
}
