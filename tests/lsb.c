#include "greatest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "handlers/image.h"
#include "utils/file.h"
#include "structs/image.h"
#include "stb_image.h"
#include "helpers.h"

#define TEST_IMAGE_WIDTH 8
#define TEST_IMAGE_HEIGHT 8
#define TEST_IMAGE_CHANNELS 4

TEST lsb_encode_roundtrip(void) {
    char *input_path = create_test_png(TEST_IMAGE_WIDTH, TEST_IMAGE_HEIGHT, TEST_IMAGE_CHANNELS);
    ASSERT(input_path != NULL);

    const char *secret = "SECRET";
    size_t secret_len = strlen(secret);

    char output_path[] = "/tmp/test_lsb_output_XXXXXX";
    int fd = mkstemp(output_path);
    ASSERT(fd >= 0);
    close(fd);

    enum FileType type = get_file_type(input_path);
    ASSERT_EQ(TYPE_PNG_IMAGE, type);

    int status = encode_image(input_path, type, output_path, (unsigned char *)secret, secret_len);
    ASSERT_EQ(0, status);

    int width, height, channels;
    unsigned char *output_pixels = stbi_load(output_path, &width, &height, &channels, 0);
    ASSERT(output_pixels != NULL);

    ASSERT_EQ(TEST_IMAGE_WIDTH, width);
    ASSERT_EQ(TEST_IMAGE_HEIGHT, height);
    ASSERT_EQ(TEST_IMAGE_CHANNELS, channels);

    size_t total_bytes = (size_t)width * height * (size_t)channels;
    ASSERT(total_bytes >= secret_len * 8);

    for (size_t i = 0; i < secret_len; i++) {
        unsigned char decoded = 0;
        for (int bit = 0; bit < 8; bit++) {
            size_t pixel_index = i * 8 + bit;
            unsigned char pixel_lsb = output_pixels[pixel_index] & 1;
            decoded |= (unsigned char)(pixel_lsb << bit);
        }
        ASSERT_EQ(((unsigned char *)secret)[i], decoded);
    }

    stbi_image_free(output_pixels);
    unlink(input_path);
    unlink(output_path);
    free(input_path);
    PASS();
}

SUITE(lsb_suite) {
    RUN_TEST(lsb_encode_roundtrip);
}
