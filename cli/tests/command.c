#include "../src/cmd/command.h"
#include "greatest.h"
#include <stdlib.h>
#include <string.h>

TEST compare_commands_returns_neg_for_less(void) {
    const char *name_a = "alpha";
    const char *name_b = "beta";
    struct Command cmd_a = {.name = name_a};
    struct Command cmd_b = {.name = name_b};
    const struct Command *a = &cmd_a;
    const struct Command *b = &cmd_b;

    ASSERT(compare_commands(&a, &b) < 0);
    PASS();
}

TEST compare_commands_returns_pos_for_greater(void) {
    const char *name_a = "zeta";
    const char *name_b = "alpha";
    struct Command cmd_a = {.name = name_a};
    struct Command cmd_b = {.name = name_b};
    const struct Command *a = &cmd_a;
    const struct Command *b = &cmd_b;

    ASSERT(compare_commands(&a, &b) > 0);
    PASS();
}

TEST compare_commands_returns_zero_for_equal(void) {
    const char *name = "encode";
    struct Command cmd = {.name = name};
    const struct Command *a = &cmd;
    const struct Command *b = &cmd;

    ASSERT_EQ(0, compare_commands(&a, &b));
    PASS();
}

/*
 * The registry is built by the linker, so these check the real table rather
 * than a fixture: every COMMAND() in the binary has to be reachable by the
 * .name it declares, whatever order the linker laid the entries out in.
 */
TEST find_command_resolves_every_registered_command(void) {
    ASSERT(COMMAND_COUNT > 0);

    for (size_t i = 0; i < COMMAND_COUNT; i++) {
        const struct Command *registered = __start_commands[i];

        ASSERT(registered != NULL);
        ASSERT(registered->name != NULL);
        ASSERT_EQ(registered, find_command(registered->name));
    }

    PASS();
}

TEST find_command_returns_null_for_an_unknown_name(void) {
    ASSERT_EQ(NULL, find_command("definitely-not-a-command"));
    PASS();
}

TEST find_command_returns_null_for_no_name(void) {
    ASSERT_EQ(NULL, find_command(NULL));
    PASS();
}

SUITE(command_suite) {
    RUN_TEST(compare_commands_returns_neg_for_less);
    RUN_TEST(compare_commands_returns_pos_for_greater);
    RUN_TEST(compare_commands_returns_zero_for_equal);
    RUN_TEST(find_command_resolves_every_registered_command);
    RUN_TEST(find_command_returns_null_for_an_unknown_name);
    RUN_TEST(find_command_returns_null_for_no_name);
}
