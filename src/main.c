#include "cli/usage.h"
#include "cmd/command.h"
#include "log.h"
#include <sodium.h>
#include <string.h>

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

    const char *cmd_name = argv[1];
    const struct Command *cmd = find_command(cmd_name);
    argc -= 2;
    argv += 2;

    printf("%d", argc);
    if (!cmd) {
        ERROR("Subcommand %s was not found", cmd_name);
        return 1;
    }

    int exec_status = cmd->exec(argc, argv);
    if (exec_status == EXEC_USAGE_ERROR) {
        printf("%s", cmd->usage);
        return 1;
    }

    if (exec_status == EXEC_GENERIC_ERROR) {
        ERROR("An error accrued while running %s subcommand", cmd->name);
        return 1;
    }

    return 0;
}
