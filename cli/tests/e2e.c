#include "greatest.h"
#include "helpers.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef HH_BINARY
#error "HH_BINARY must point at the hh executable"
#endif

#define RUN_OUTPUT_MAX 8192
#define SECRET_LEN 200
/* Matches PASSPHRASE_MAX in the CLI, plus room for the trailing newline. */
#define PASSPHRASE_LINE_MAX 258

struct Run {
    int exit_code;
    char output[RUN_OUTPUT_MAX];
};

static int run_hh(const char *const *args, const char *input, struct Run *run) {
    int in_pipe[2];
    int out_pipe[2];

    run->exit_code = -1;
    run->output[0] = '\0';

    if (pipe(in_pipe) != 0)
        return -1;

    if (pipe(out_pipe) != 0) {
        close(in_pipe[0]);
        close(in_pipe[1]);
        return -1;
    }

    //argv[0] plus the caller's arguments plus the NULL terminator.
    size_t count = 0;
    while (args[count])
        count++;

    char **argv = calloc(count + 2, sizeof(*argv));
    if (!argv) {
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);
        return -1;
    }

    argv[0] = (char *)HH_BINARY;
    for (size_t i = 0; i < count; i++)
        argv[i + 1] = (char *)args[i];

    pid_t pid = fork();
    if (pid < 0) {
        free(argv);
        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);
        return -1;
    }

    if (pid == 0) {
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(out_pipe[1], STDERR_FILENO);

        close(in_pipe[0]);
        close(in_pipe[1]);
        close(out_pipe[0]);
        close(out_pipe[1]);

        execv(HH_BINARY, argv);
        _exit(127);
    }

    free(argv);
    close(in_pipe[0]);
    close(out_pipe[1]);

    if (input && *input) {
        size_t len = strlen(input);
        size_t written = 0;

        while (written < len) {
            ssize_t n = write(in_pipe[1], input + written, len - written);
            if (n <= 0)
                break;
            written += (size_t)n;
        }
    }
    close(in_pipe[1]);

    size_t total = 0;
    for (;;) {
        char scratch[1024];
        ssize_t n = read(out_pipe[0], scratch, sizeof(scratch));
        if (n <= 0)
            break;

        size_t room = sizeof(run->output) - 1 - total;
        size_t copy = ((size_t)n < room) ? (size_t)n : room;
        memcpy(run->output + total, scratch, copy);
        total += copy;
    }
    run->output[total] = '\0';
    close(out_pipe[0]);

    int status = 0;
    if (waitpid(pid, &status, 0) < 0)
        return -1;

    run->exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return 0;
}

static char last_output[RUN_OUTPUT_MAX];

static const char *said(const struct Run *run) {
    snprintf(last_output, sizeof(last_output), "%s", run->output);
    return last_output;
}

static char *temp_path(const char *tag) {
    char path[64];
    snprintf(path, sizeof(path), "/tmp/hh_e2e_%s_XXXXXX", tag);

    int fd = mkstemp(path);
    if (fd < 0)
        return NULL;

    close(fd);
    return strdup(path);
}

static char *write_secret(unsigned char *expected) {
    for (size_t i = 0; i < SECRET_LEN; i++)
        expected[i] = (unsigned char)(i * 11 + 5);

    char *path = temp_path("secret");
    if (!path)
        return NULL;

    FILE *file = fopen(path, "wb");
    if (!file) {
        free(path);
        return NULL;
    }

    size_t written = fwrite(expected, 1, SECRET_LEN, file);
    if (fclose(file) != 0 || written != SECRET_LEN) {
        unlink(path);
        free(path);
        return NULL;
    }

    return path;
}

static int file_matches(const char *path, const unsigned char *expected, size_t expected_len) {
    FILE *file = fopen(path, "rb");
    if (!file)
        return 0;

    unsigned char *actual = malloc(expected_len + 1);
    if (!actual) {
        fclose(file);
        return 0;
    }

    size_t len = fread(actual, 1, expected_len + 1, file);
    fclose(file);

    int same = (len == expected_len) && memcmp(actual, expected, expected_len) == 0;
    free(actual);

    return same;
}

static enum greatest_test_res round_trip(const char *codec, const char *passphrase) {
    unsigned char expected[SECRET_LEN];

    char *carrier = create_test_png(64, 64, 3);
    ASSERT(carrier != NULL);

    char *secret = write_secret(expected);
    ASSERT(secret != NULL);

    char *stego = temp_path("stego");
    ASSERT(stego != NULL);

    char *recovered = temp_path("recovered");
    ASSERT(recovered != NULL);

    char stdin_line[PASSPHRASE_LINE_MAX];
    snprintf(stdin_line, sizeof(stdin_line), "%s\n", passphrase);

    const char *encode_args[] = {"encode", carrier, secret, "-o", stego, "-c", codec, NULL};
    struct Run encode = {0};
    ASSERT_EQ(0, run_hh(encode_args, stdin_line, &encode));
    ASSERT_EQm(said(&encode), 0, encode.exit_code);

    const char *decode_args[] = {"decode", stego, "-o", recovered, NULL};
    struct Run decode = {0};
    ASSERT_EQ(0, run_hh(decode_args, stdin_line, &decode));
    ASSERT_EQm(said(&decode), 0, decode.exit_code);

    ASSERT(file_matches(recovered, expected, SECRET_LEN));

    unlink(carrier);
    unlink(secret);
    unlink(stego);
    unlink(recovered);
    free(carrier);
    free(secret);
    free(stego);
    free(recovered);

    PASS();
}

TEST round_trips_plain_replacement(void) {
    CHECK_CALL(round_trip("lsbr", ""));
    PASS();
}

TEST round_trips_plain_matching(void) {
    CHECK_CALL(round_trip("lsbm", ""));
    PASS();
}

TEST round_trips_encrypted(void) {
    CHECK_CALL(round_trip("lsbr", "correct horse battery staple"));
    PASS();
}

TEST decode_fails_with_the_wrong_passphrase(void) {
    unsigned char expected[SECRET_LEN];

    char *carrier = create_test_png(64, 64, 3);
    ASSERT(carrier != NULL);

    char *secret = write_secret(expected);
    ASSERT(secret != NULL);

    char *stego = temp_path("stego");
    ASSERT(stego != NULL);

    char *recovered = temp_path("recovered");
    ASSERT(recovered != NULL);

    const char *encode_args[] = {"encode", carrier, secret, "-o", stego, "-c", "lsbr", NULL};
    struct Run encode = {0};
    ASSERT_EQ(0, run_hh(encode_args, "right one\n", &encode));
    ASSERT_EQm(said(&encode), 0, encode.exit_code);

    const char *decode_args[] = {"decode", stego, "-o", recovered, NULL};
    struct Run decode = {0};
    ASSERT_EQ(0, run_hh(decode_args, "wrong one\n", &decode));
    ASSERT(decode.exit_code != 0);

    unlink(carrier);
    unlink(secret);
    unlink(stego);
    unlink(recovered);
    free(carrier);
    free(secret);
    free(stego);
    free(recovered);
    PASS();
}

TEST decode_fails_on_an_image_without_a_container(void) {
    char *carrier = create_test_png(64, 64, 3);
    ASSERT(carrier != NULL);

    char *recovered = temp_path("recovered");
    ASSERT(recovered != NULL);

    const char *args[] = {"decode", carrier, "-o", recovered, NULL};
    struct Run decode = {0};
    ASSERT_EQ(0, run_hh(args, "\n", &decode));
    ASSERT(decode.exit_code != 0);

    unlink(carrier);
    unlink(recovered);
    free(carrier);
    free(recovered);
    PASS();
}

TEST encode_fails_on_a_missing_target(void) {
    unsigned char expected[SECRET_LEN];

    char *secret = write_secret(expected);
    ASSERT(secret != NULL);

    char *stego = temp_path("stego");
    ASSERT(stego != NULL);

    const char *args[] = {"encode", "/tmp/hh_e2e_no_such_carrier.png", secret, "-o", stego, "-c", "lsbr", NULL};
    struct Run encode = {0};
    ASSERT_EQ(0, run_hh(args, "\n", &encode));
    ASSERT(encode.exit_code != 0);

    unlink(secret);
    unlink(stego);
    free(secret);
    free(stego);
    PASS();
}

TEST encode_rejects_an_unknown_codec(void) {
    unsigned char expected[SECRET_LEN];

    char *carrier = create_test_png(64, 64, 3);
    ASSERT(carrier != NULL);

    char *secret = write_secret(expected);
    ASSERT(secret != NULL);

    char *stego = temp_path("stego");
    ASSERT(stego != NULL);

    const char *args[] = {"encode", carrier, secret, "-o", stego, "-c", "nosuchcodec", NULL};
    struct Run encode = {0};
    ASSERT_EQ(0, run_hh(args, "\n", &encode));
    ASSERT(encode.exit_code != 0);

    unlink(carrier);
    unlink(secret);
    unlink(stego);
    free(carrier);
    free(secret);
    free(stego);
    PASS();
}

TEST inspect_reports_the_image_geometry(void) {
    char *carrier = create_test_png(64, 64, 3);
    ASSERT(carrier != NULL);

    const char *args[] = {"inspect", carrier, "-l", "2", NULL};
    struct Run inspect = {0};
    ASSERT_EQ(0, run_hh(args, "", &inspect));
    ASSERT_EQm(said(&inspect), 0, inspect.exit_code);
    ASSERT(strstr(inspect.output, "64x64") != NULL);

    unlink(carrier);
    free(carrier);
    PASS();
}

TEST no_arguments_prints_usage(void) {
    const char *args[] = {NULL};
    struct Run run = {0};

    ASSERT_EQ(0, run_hh(args, "", &run));
    ASSERT(run.exit_code != 0);
    ASSERT(strstr(run.output, "[subcommand]") != NULL);
    PASS();
}

TEST verbose_keeps_the_program_name_in_usage(void) {
    const char *args[] = {"--verbose", NULL};
    struct Run run = {0};

    ASSERT_EQ(0, run_hh(args, "", &run));
    ASSERT(run.exit_code != 0);
    ASSERT(strstr(run.output, "[subcommand]") != NULL);
    ASSERT(strstr(run.output, "--verbose [") == NULL);
    ASSERT(strstr(run.output, HH_BINARY) != NULL);
    PASS();
}

TEST an_unknown_subcommand_fails(void) {
    const char *args[] = {"definitely-not-a-command", "/tmp", NULL};
    struct Run run = {0};

    ASSERT_EQ(0, run_hh(args, "", &run));
    ASSERT(run.exit_code != 0);
    ASSERT(strstr(run.output, "definitely-not-a-command") != NULL);
    PASS();
}

SUITE(e2e_suite) {
    signal(SIGPIPE, SIG_IGN);

    RUN_TEST(round_trips_plain_replacement);
    RUN_TEST(round_trips_plain_matching);
    RUN_TEST(round_trips_encrypted);
    RUN_TEST(decode_fails_with_the_wrong_passphrase);
    RUN_TEST(decode_fails_on_an_image_without_a_container);
    RUN_TEST(encode_fails_on_a_missing_target);
    RUN_TEST(encode_rejects_an_unknown_codec);
    RUN_TEST(inspect_reports_the_image_geometry);
    RUN_TEST(no_arguments_prints_usage);
    RUN_TEST(verbose_keeps_the_program_name_in_usage);
    RUN_TEST(an_unknown_subcommand_fails);
}
