#include "greatest.h"

#include "helpers.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/analysis/inspect.h"

TEST loads_an_image_into_a_pixel_buffer(void) {
    char *path = create_test_png(8, 4, 3);
    ASSERT(path != NULL);

    struct PixelBuffer pixels;
    ASSERT_EQ(0, pixels_load(path, &pixels));

    ASSERT_EQ(8, pixels.width);
    ASSERT_EQ(4, pixels.height);
    ASSERT_EQ(3, pixels.channels);
    ASSERT_EQ((size_t)(8 * 4 * 3), pixels.len);
    ASSERT(pixels.samples != NULL);

    pixels_free(&pixels);
    ASSERT(pixels.samples == NULL);

    unlink(path);
    free(path);
    PASS();
}

TEST rejects_a_file_that_is_not_an_image(void) {
    struct PixelBuffer pixels;

    ASSERT_EQ(-1, pixels_load("/nonexistent/not-an-image.png", &pixels));
    ASSERT_EQ(-1, pixels_load(NULL, &pixels));
    PASS();
}

TEST gathers_the_low_bits_low_first(void) {
    unsigned char samples[16] = {0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0};

    struct PixelBuffer pixels = {
        .samples = samples,
        .len = sizeof(samples),
        .width = 16,
        .height = 1,
        .channels = 1,
    };

    struct LsbStream stream;
    ASSERT_EQ(0, inspect_lsb(&pixels, NO_LIMIT, &stream));

    ASSERT_EQ((size_t)2, stream.len);
    ASSERT_EQ(0xB2, stream.bytes[0]);
    ASSERT_EQ(0x01, stream.bytes[1]);

    lsb_stream_free(&stream);
    PASS();
}

TEST skips_the_alpha_channel(void) {
    unsigned char samples[32];
    memset(samples, 0, sizeof(samples));

    for (size_t i = 0; i < sizeof(samples); i++)
        samples[i] = (i % 4 == 3) ? 0 : 1;

    struct PixelBuffer pixels = {
        .samples = samples,
        .len = sizeof(samples),
        .width = 8,
        .height = 1,
        .channels = 4,
    };

    struct LsbStream stream;
    ASSERT_EQ(0, inspect_lsb(&pixels, NO_LIMIT, &stream));

    ASSERT_EQ((size_t)3, stream.len);
    ASSERT_EQ(0xFF, stream.bytes[0]);

    lsb_stream_free(&stream);
    PASS();
}

TEST honours_the_row_limit(void) {
    unsigned char samples[256];
    memset(samples, 0, sizeof(samples));

    struct PixelBuffer pixels = {
        .samples = samples,
        .len = sizeof(samples),
        .width = 256,
        .height = 1,
        .channels = 1,
    };

    struct LsbStream stream;
    ASSERT_EQ(0, inspect_lsb(&pixels, 4, &stream));
    ASSERT_EQ((size_t)4, stream.len);
    lsb_stream_free(&stream);

    ASSERT_EQ(0, inspect_lsb(&pixels, NO_LIMIT, &stream));
    ASSERT_EQ((size_t)32, stream.len);
    lsb_stream_free(&stream);

    ASSERT_EQ(0, inspect_lsb(&pixels, 4096, &stream));
    ASSERT_EQ((size_t)32, stream.len);
    lsb_stream_free(&stream);

    PASS();
}

TEST formats_a_row_three_ways(void) {
    struct InspectRow row;

    inspect_row(0xB2, &row);
    ASSERT_STR_EQ("10110010", row.binary);
    ASSERT_STR_EQ("0xB2", row.hex);
    ASSERT_STR_EQ(".", row.ascii);

    inspect_row('A', &row);
    ASSERT_STR_EQ("01000001", row.binary);
    ASSERT_STR_EQ("0x41", row.hex);
    ASSERT_STR_EQ("A", row.ascii);

    PASS();
}

TEST classifies_bytes_for_colouring(void) {
    ASSERT_EQ(BYTE_ZERO, classify_byte(0x00));
    ASSERT_EQ(BYTE_FILLED, classify_byte(0xFF));
    ASSERT_EQ(BYTE_WHITESPACE, classify_byte(' '));
    ASSERT_EQ(BYTE_WHITESPACE, classify_byte('\n'));
    ASSERT_EQ(BYTE_PRINTABLE, classify_byte('A'));
    ASSERT_EQ(BYTE_CONTROL, classify_byte(0x01));
    ASSERT_EQ(BYTE_CONTROL, classify_byte(0x7F));
    ASSERT_EQ(BYTE_OTHER, classify_byte(0x80));
    PASS();
}

SUITE(inspect_suite) {
    RUN_TEST(loads_an_image_into_a_pixel_buffer);
    RUN_TEST(rejects_a_file_that_is_not_an_image);
    RUN_TEST(gathers_the_low_bits_low_first);
    RUN_TEST(skips_the_alpha_channel);
    RUN_TEST(honours_the_row_limit);
    RUN_TEST(formats_a_row_three_ways);
    RUN_TEST(classifies_bytes_for_colouring);
}
