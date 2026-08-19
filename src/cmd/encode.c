#include "cmd/encode.h"
#include "cmd/command.h"

int exec(int argc, const char *argv[]) {
    if (argc < 4)
        return EXEC_USAGE_ERROR;

    const char *target = argv[0];

    return EXEC_OK;
}

static const struct Command encode_cmd = {
    .name = ENCODE_CMD_NAME,
    .description = "todo",
    .usage = "todo",
    .exec = exec
};

COMMAND(encode_cmd);
