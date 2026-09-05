#include "greatest.h"
#include "helpers.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/analysis/dct.h"
#include "core/codecs/codec.h"
#include "core/codecs/container.h"
#include "core/fs/file.h"
#include "core/handlers/image.h"

#define CARRIER_SIZE 128
#define CARRIER_QUALITY 95

static char *temp_path(void) {
    char path[] = "/tmp/test_dct_inspect_XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0)
        return NULL;

    close(fd);
    return strdup(path);
}

TEST loads_the_coefficients_of_a_jpeg(void) {
    char *path = create_test_jpg(CARRIER_SIZE, CARRIER_SIZE, CARRIER_QUALITY);
    ASSERT(path != NULL);

    struct DctCoefficients coefficients;
    ASSERT_EQ(0, dct_load(path, &coefficients));

    ASSERT_EQ(CARRIER_SIZE, coefficients.width);
    ASSERT_EQ(CARRIER_SIZE, coefficients.height);
    ASSERT_EQ(3, coefficients.components);
    ASSERT(coefficients.count > 0);
    ASSERT(coefficients.values != NULL);

    /* The codec never hands out a 0 or a 1, so neither may reach the stream. */
    for (size_t i = 0; i < coefficients.count; i++) {
        ASSERT(coefficients.values[i] != 0);
        ASSERT(coefficients.values[i] != 1);
    }

    dct_free(&coefficients);
    ASSERT(coefficients.values == NULL);

    unlink(path);
    free(path);
    PASS();
}

TEST refuses_what_is_not_a_jpeg(void) {
    char *path = create_test_png(32, 32, 3);
    ASSERT(path != NULL);

    struct DctCoefficients coefficients;
    ASSERT_EQ(-1, dct_load(path, &coefficients));
    ASSERT_EQ(-1, dct_load("/nonexistent/nope.jpg", &coefficients));
    ASSERT_EQ(-1, dct_load(NULL, &coefficients));

    unlink(path);
    free(path);
    PASS();
}

TEST gathers_the_low_bits_low_first(void) {
    short values[16] = {2, 3, 2, 2, 3, 3, 2, 3, 3, 2, 2, 2, 2, 2, 2, 2};

    struct DctCoefficients coefficients = {
        .values = values,
        .count = 16,
        .width = 8,
        .height = 8,
        .components = 1,
    };

    struct DctStream stream;
    ASSERT_EQ(0, inspect_dct(&coefficients, NO_LIMIT, &stream));

    ASSERT_EQ((size_t)2, stream.len);
    ASSERT_EQ(0xB2, stream.bytes[0]);
    ASSERT_EQ(0x01, stream.bytes[1]);

    dct_stream_free(&stream);
    PASS();
}

/* Negative coefficients are usable, and it is the low bit of the stored value
 * that counts, not its sign. */
TEST reads_negative_coefficients(void) {
    short values[8] = {-3, -2, -2, -2, -2, -2, -2, -2};

    struct DctCoefficients coefficients = {.values = values, .count = 8, .width = 8, .height = 8, .components = 1};

    struct DctStream stream;
    ASSERT_EQ(0, inspect_dct(&coefficients, NO_LIMIT, &stream));

    ASSERT_EQ((size_t)1, stream.len);
    ASSERT_EQ(0x01, stream.bytes[0]);

    dct_stream_free(&stream);
    PASS();
}

TEST honours_the_row_limit(void) {
    short values[256];
    for (size_t i = 0; i < 256; i++)
        values[i] = 2;

    struct DctCoefficients coefficients = {.values = values, .count = 256, .width = 8, .height = 8, .components = 1};

    struct DctStream stream;
    ASSERT_EQ(0, inspect_dct(&coefficients, 4, &stream));
    ASSERT_EQ((size_t)4, stream.len);
    dct_stream_free(&stream);

    ASSERT_EQ(0, inspect_dct(&coefficients, NO_LIMIT, &stream));
    ASSERT_EQ((size_t)32, stream.len);
    dct_stream_free(&stream);

    ASSERT_EQ(0, inspect_dct(&coefficients, 4096, &stream));
    ASSERT_EQ((size_t)32, stream.len);
    dct_stream_free(&stream);

    PASS();
}

/* The whole point of the view: what the codec wrote is what it reads back. */
TEST shows_the_container_a_dct_encode_left_behind(void) {
    char *carrier = create_test_jpg(CARRIER_SIZE, CARRIER_SIZE, CARRIER_QUALITY);
    ASSERT(carrier != NULL);

    char *output = temp_path();
    ASSERT(output != NULL);

    unsigned char payload[64];
    for (size_t i = 0; i < sizeof(payload); i++)
        payload[i] = (unsigned char)(i * 3 + 1);

    struct ImageCtx ctx = {
        .source_file = carrier,
        .output_file = output,

        .image_type = TYPE_JPEG_IMAGE,
        .codec_type = CODEC_DCT,

        .passphrase = NULL,
    };

    ASSERT_EQ(0, encode_image(&ctx, payload, sizeof(payload)));

    struct DctCoefficients coefficients;
    ASSERT_EQ(0, dct_load(output, &coefficients));

    struct DctStream stream;
    ASSERT_EQ(0, inspect_dct(&coefficients, NO_LIMIT, &stream));

    ASSERT(stream.len >= HUSH_MAGIC_LEN);
    ASSERT_MEM_EQ(HUSH_MAGIC, stream.bytes, HUSH_MAGIC_LEN);

    dct_stream_free(&stream);
    dct_free(&coefficients);

    unlink(carrier);
    unlink(output);
    free(carrier);
    free(output);
    PASS();
}

SUITE(dct_inspect_suite) {
    RUN_TEST(loads_the_coefficients_of_a_jpeg);
    RUN_TEST(refuses_what_is_not_a_jpeg);
    RUN_TEST(gathers_the_low_bits_low_first);
    RUN_TEST(reads_negative_coefficients);
    RUN_TEST(honours_the_row_limit);
    RUN_TEST(shows_the_container_a_dct_encode_left_behind);
}
