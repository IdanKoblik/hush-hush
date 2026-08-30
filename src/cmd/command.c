#include "cmd/command.h"
#include <stdlib.h>
#include <string.h>

int compare_commands(const void *a, const void *b) {
    const struct Command *const *ca = a;
    const struct Command *const *cb = b;

    return strcmp((*ca)->name, (*cb)->name);
}

const struct Command *find_command(const char *name) {
    struct Command key = {
        .name = name,
    };

    const struct Command *key_ptr = &key;

    const struct Command **result = bsearch(&key_ptr, __start_commands, COMMAND_COUNT, sizeof(*__start_commands), compare_commands);

    if (result == NULL)
        return NULL;

    return *result;
}
