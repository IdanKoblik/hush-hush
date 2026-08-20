#include "greatest.h"
#include <stdlib.h>
#include <string.h>
#include "cmd/command.h"

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

SUITE(command_suite) {
    RUN_TEST(compare_commands_returns_neg_for_less);
    RUN_TEST(compare_commands_returns_pos_for_greater);
    RUN_TEST(compare_commands_returns_zero_for_equal);
}
