#ifndef COMMAND_H_
#define COMMAND_H_

#include <stddef.h>

enum {
    EXEC_OK = 0,
    EXEC_GENERIC_ERROR = -1,
    EXEC_USAGE_ERROR = -2,
};

struct Command {
    const char *name;
    const char *description;
    const char *usage;

    int (*exec)(int argc, char *argv[]);
};

#define COMMAND(cmd) \
    static const struct Command *__attribute__((used, section("commands." #cmd))) command_##cmd = &(cmd)

extern const struct Command *__start_commands[];
extern const struct Command *__stop_commands[];

#define COMMAND_COUNT ((size_t)(__stop_commands - __start_commands))

int compare_commands(const void *a, const void *b);
const struct Command *find_command(const char *name);

#endif // COMMAND_H_
