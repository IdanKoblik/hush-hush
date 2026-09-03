#include "../src/usage.h"
#include "greatest.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CAPTURE_STDOUT(output, code)                               \
    do {                                                           \
        int pipefd[2];                                             \
        int piped = pipe(pipefd);                                  \
        ASSERT_EQ(0, piped);                                       \
                                                                   \
        fflush(stdout);                                            \
        int old_stdout = dup(STDOUT_FILENO);                       \
        dup2(pipefd[1], STDOUT_FILENO);                            \
        close(pipefd[1]);                                          \
                                                                   \
        do {                                                       \
            code;                                                  \
        } while (0);                                               \
                                                                   \
        fflush(stdout);                                            \
        dup2(old_stdout, STDOUT_FILENO);                           \
        close(old_stdout);                                         \
                                                                   \
        ssize_t n = read(pipefd[0], (output), sizeof(output) - 1); \
        if (n < 0)                                                 \
            n = 0;                                                 \
        (output)[n] = '\0';                                        \
        close(pipefd[0]);                                          \
    } while (0)

TEST usage_exec_exists(void) {
    char output[1024];
    CAPTURE_STDOUT(output, print_usage("test"));

    ASSERT_STR_EQ("test [verbose (optional)] [subcommand] <target> [options]\n", output);
    PASS();
}

TEST usage_exec_absent(void) {
    char output[1024];
    CAPTURE_STDOUT(output, print_usage(NULL));

    ASSERT_STR_EQ("./hh [verbose (optional)] [subcommand] <target> [options]\n", output);
    PASS();
}

SUITE(usage_suite) {
    RUN_TEST(usage_exec_exists);
    RUN_TEST(usage_exec_absent);
}
