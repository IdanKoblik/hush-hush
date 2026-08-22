#include "greatest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "utils/file.h"

TEST get_file_data_reads_file_contents(void) {
    const char *path = "/tmp/test_file_data_XXXXXX";
    int fd = mkstemp((char *)path);
    ASSERT(fd >= 0);

    const char *content = "hello world";
    ASSERT(write(fd, content, strlen(content)) == (ssize_t)strlen(content));
    close(fd);

    unsigned char *data = NULL;
    size_t data_len = 0;
    int result = get_file_raw_data(path, &data, &data_len);

    ASSERT_EQ(0, result);
    ASSERT(data != NULL);
    ASSERT_EQ(strlen(content), data_len);
    ASSERT_STR_EQ(content, (const char *)data);

    free(data);
    unlink(path);
    PASS();
}

TEST get_file_data_returns_neg_for_nonexistent(void) {
    unsigned char *data = NULL;
    size_t data_len = 0;
    int result = get_file_raw_data("/tmp/nonexistent_file_hush_hush", &data, &data_len);

    ASSERT(result < 0);
    PASS();
}

TEST get_file_data_reads_empty_file(void) {
    const char *path = "/tmp/test_empty_file_XXXXXX";
    int fd = mkstemp((char *)path);
    ASSERT(fd >= 0);
    close(fd);

    unsigned char *data = NULL;
    size_t data_len = 0;
    int result = get_file_raw_data(path, &data, &data_len);

    ASSERT_EQ(0, result);
    ASSERT(data != NULL);
    ASSERT_EQ(0, data_len);

    free(data);
    unlink(path);
    PASS();
}

TEST get_file_type_returns_not_found_for_nonexistent(void) {
    enum FileType type = get_file_type("/tmp/nonexistent_file_hush_hush");
    ASSERT(type == TYPE_UNKNOWN || type == TYPE_NOT_FOUND);
    PASS();
}

SUITE(file_suite) {
    RUN_TEST(get_file_data_reads_file_contents);
    RUN_TEST(get_file_data_returns_neg_for_nonexistent);
    RUN_TEST(get_file_data_reads_empty_file);
    RUN_TEST(get_file_type_returns_not_found_for_nonexistent);
}
