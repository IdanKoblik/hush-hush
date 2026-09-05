#include "command.h"
#include "stdio.h"

static int exec(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    printf("Subcommands:\n");
    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        const struct Command *cmd = __start_commands[i];
        printf("  - %s: %s\n", cmd->name, cmd->description);
    }

    return EXEC_OK;
}

static const struct Command help_command = {
    .name = "help",
    .description = "Show list of available subcommands",
    .usage = "",
    .exec = exec
};

COMMAND(help_command);
