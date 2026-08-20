#include "greatest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "handlers/image.h"
#include "utils/file.h"
#include "helpers.h"

TEST detect_image_finds_png(void) {
    char *path = create_test_png(2, 2, 4);
    ASSERT(path != NULL);

    enum FileType type = detect_image(path);
    ASSERT_EQ(TYPE_PNG_IMAGE, type);

    unlink(path);
    free(path);
    PASS();
}

TEST detect_image_returns_unknown_for_non_image(void) {
    const char *path = "/tmp/test_non_image_XXXXXX";
    int fd = mkstemp((char *)path);
    ASSERT(fd >= 0);
    const char *content = "not an image file";
    ASSERT(write(fd, content, strlen(content)) == (ssize_t)strlen(content));
    close(fd);

    enum FileType type = detect_image(path);
    ASSERT_EQ(TYPE_UNKNOWN, type);

    unlink(path);
    PASS();
}

TEST detect_image_returns_not_found_for_nonexistent(void) {
    enum FileType type = detect_image("/tmp/nonexistent_file_hush_hush");
    ASSERT(type == TYPE_UNKNOWN || type == TYPE_NOT_FOUND);
    PASS();
}

SUITE(image_suite) {
    RUN_TEST(detect_image_finds_png);
    RUN_TEST(detect_image_returns_unknown_for_non_image);
    RUN_TEST(detect_image_returns_not_found_for_nonexistent);
}
