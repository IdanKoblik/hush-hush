#include <sodium.h>
#include <string.h>
#include "cli/usage.h"
#include "cmd/command.h"
#include "log.h"

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "--verbose") == 0) {
        verbose = 1;
        argc--;
        argv++;
    }

    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    DEBUG("Loaded %zu commands", COMMAND_COUNT);

    if (sodium_init() < 0) {
        ERROR("Failed to initialise libsodium");
        return 1;
    }

    return 0;
}
