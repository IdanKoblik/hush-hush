#include "command.h"

#include <stdlib.h>
#include <string.h>

int compare_commands(const void *a, const void *b) {
    const struct Command *const *ca = a;
    const struct Command *const *cb = b;

    return strcmp((*ca)->name, (*cb)->name);
}

const struct Command *find_command(const char *name) {
    if (!name)
        return NULL;

    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        if (strcmp(__start_commands[i]->name, name) == 0)
            return __start_commands[i];
    }

    return NULL;
}
