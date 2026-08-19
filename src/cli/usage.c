#include "cli/usage.h"
#include <stdio.h>

void print_usage(const char *exec) {
    printf("%s [verbose (optional)] [subcommand] <target> [options]\n", (exec == NULL ? "./hh" : exec));
}
