#include "greatest.h"
#include "helpers.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "core/analysis/stats.h"
#include "core/codecs/codec.h"
#include "core/handlers/image.h"

#define CARRIER_SIZE 128
#define CARRIER_QUALITY 95

/* 128 x 128 x 3 colour samples, one bit each. */
#define CARRIER_CAPACITY (CARRIER_SIZE * CARRIER_SIZE * 3 / 8)

/* Comfortably under the capacity above once the preamble and the end marker
 * are paid for, but close enough to it that almost every low bit is written. */
#define PAYLOAD_LEN 6000

static unsigned int next_random(unsigned int *state) {
    *state = *state * 1103515245u + 12345u;

    return (*state >> 16) & 0x7FFFu;
}

static char *temp_path(void) {
    char path[] = "/tmp/test_stats_XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0)
        return NULL;

    close(fd);
    return strdup(path);
}

/* Rows of one constant, even value each. Every sample has a clear low bit and
 * every neighbour along a row is identical, so an untouched carrier of this
 * shape sits at the far end of all three pixel methods. */
static char *create_banded_png(void) {
    char *path = temp_path();
    if (!path)
        return NULL;

    const size_t len = (size_t)CARRIER_SIZE * CARRIER_SIZE * 3;
    unsigned char *pixels = calloc(len, 1);
    if (!pixels) {
        free(path);
        return NULL;
    }

    for (int y = 0; y < CARRIER_SIZE; y++) {
        const unsigned char value = (unsigned char)(100 + (y / 8) * 2);

        memset(pixels + (size_t)y * CARRIER_SIZE * 3, value, (size_t)CARRIER_SIZE * 3);
    }

    const int written = stbi_write_png(path, CARRIER_SIZE, CARRIER_SIZE, 3, pixels, CARRIER_SIZE * 3);
    free(pixels);

    if (!written) {
        unlink(path);
        free(path);
        return NULL;
    }

    return path;
}

TEST the_sample_histogram_skips_alpha(void) {
    unsigned char samples[16];
    for (size_t i = 0; i < sizeof(samples); i++)
        samples[i] = (i % 4 == 3) ? 200 : 10;

    struct PixelBuffer pixels = {.samples = samples, .len = sizeof(samples), .width = 4, .height = 1, .channels = 4};

    size_t histogram[STAT_SAMPLE_LEVELS];
    ASSERT_EQ(0, stats_sample_histogram(&pixels, histogram));

    ASSERT_EQ((size_t)12, histogram[10]);
    ASSERT_EQ((size_t)0, histogram[200]);
    PASS();
}

TEST the_low_bit_ratio_is_the_share_of_odd_samples(void) {
    unsigned char samples[8] = {1, 1, 1, 2, 2, 2, 2, 2};

    struct PixelBuffer pixels = {.samples = samples, .len = sizeof(samples), .width = 8, .height = 1, .channels = 1};

    struct StatResult result;
    ASSERT_EQ(0, stats_lsb_ratio(&pixels, &result));

    ASSERT(result.applicable);
    ASSERT_IN_RANGE(37.5, result.percent, 0.001);
    ASSERT_IN_RANGE(50.0, result.reference, 0.001);
    ASSERT_EQ((size_t)8, result.population);
    PASS();
}

TEST the_low_bit_ratio_balances_at_half(void) {
    unsigned char samples[64];
    for (size_t i = 0; i < sizeof(samples); i++)
        samples[i] = (unsigned char)(100 + (i % 2));

    struct PixelBuffer pixels = {.samples = samples, .len = sizeof(samples), .width = 64, .height = 1, .channels = 1};

    struct StatResult result;
    ASSERT_EQ(0, stats_lsb_ratio(&pixels, &result));

    ASSERT_IN_RANGE(50.0, result.percent, 0.001);
    PASS();
}

/* Every pair of values holding the same count is exactly what a full payload
 * leaves behind, so the test should be as sure as it gets. */
TEST chi_square_is_certain_when_every_pair_is_level(void) {
    unsigned char samples[STAT_SAMPLE_LEVELS * 8];
    for (size_t i = 0; i < sizeof(samples); i++)
        samples[i] = (unsigned char)(i % STAT_SAMPLE_LEVELS);

    struct PixelBuffer pixels = {.samples = samples, .len = sizeof(samples), .width = (int)sizeof(samples), .height = 1, .channels = 1};

    struct StatResult result;
    ASSERT_EQ(0, stats_chi_square(&pixels, &result));

    ASSERT(result.applicable);
    ASSERT_IN_RANGE(100.0, result.percent, 0.001);
    ASSERT_EQ((size_t)(STAT_SAMPLE_LEVELS / 2), result.population);
    PASS();
}

/* And a histogram where one half of every pair is empty is the opposite: no
 * payload could have left it that way. */
TEST chi_square_sees_through_a_lopsided_histogram(void) {
    unsigned char samples[STAT_SAMPLE_LEVELS * 8];
    for (size_t i = 0; i < sizeof(samples); i++)
        samples[i] = (unsigned char)((i % (STAT_SAMPLE_LEVELS / 2)) * 2);

    struct PixelBuffer pixels = {.samples = samples, .len = sizeof(samples), .width = (int)sizeof(samples), .height = 1, .channels = 1};

    struct StatResult result;
    ASSERT_EQ(0, stats_chi_square(&pixels, &result));

    ASSERT(result.applicable);
    ASSERT(result.percent < 0.001);
    PASS();
}

/* A single populated pair spends the one degree of freedom there is. */
TEST chi_square_refuses_to_guess_from_one_pair(void) {
    unsigned char samples[64];
    memset(samples, 128, sizeof(samples));

    struct PixelBuffer pixels = {.samples = samples, .len = sizeof(samples), .width = 64, .height = 1, .channels = 1};

    struct StatResult result;
    ASSERT_EQ(0, stats_chi_square(&pixels, &result));

    ASSERT_FALSE(result.applicable);
    ASSERT_EQ((size_t)1, result.population);
    PASS();
}

TEST differences_are_taken_along_a_row_only(void) {
    unsigned char samples[4] = {0, 1, 100, 101};

    struct PixelBuffer pixels = {.samples = samples, .len = sizeof(samples), .width = 2, .height = 2, .channels = 1};

    size_t histogram[STAT_DIFFERENCE_LEVELS];
    ASSERT_EQ(0, stats_difference_histogram(&pixels, histogram));

    ASSERT_EQ((size_t)2, histogram[1 + STAT_DIFFERENCE_ZERO]);

    /* 100 - 1, the step from the end of one row to the start of the next. */
    ASSERT_EQ((size_t)0, histogram[99 + STAT_DIFFERENCE_ZERO]);
    PASS();
}

TEST the_difference_histogram_keeps_the_sign(void) {
    unsigned char samples[4] = {10, 7, 7, 12};

    struct PixelBuffer pixels = {.samples = samples, .len = sizeof(samples), .width = 4, .height = 1, .channels = 1};

    size_t histogram[STAT_DIFFERENCE_LEVELS];
    ASSERT_EQ(0, stats_difference_histogram(&pixels, histogram));

    ASSERT_EQ((size_t)1, histogram[-3 + STAT_DIFFERENCE_ZERO]);
    ASSERT_EQ((size_t)1, histogram[0 + STAT_DIFFERENCE_ZERO]);
    ASSERT_EQ((size_t)1, histogram[5 + STAT_DIFFERENCE_ZERO]);
    PASS();
}

/* Identical neighbours mean no odd difference anywhere, which is as far from
 * a carrier full of random bits as the method can report. */
TEST hod_bottoms_out_on_a_flat_carrier(void) {
    unsigned char samples[64];
    memset(samples, 128, sizeof(samples));

    struct PixelBuffer pixels = {.samples = samples, .len = sizeof(samples), .width = 8, .height = 8, .channels = 1};

    struct StatResult result;
    ASSERT_EQ(0, stats_hod(&pixels, &result));

    ASSERT(result.applicable);
    ASSERT_IN_RANGE(0.0, result.percent, 0.001);
    ASSERT_IN_RANGE(50.0, result.reference, 0.001);
    ASSERT_EQ((size_t)(8 * 7), result.population);
    PASS();
}

/* The same carrier with random low bits written over it. The parity of a
 * difference is now a coin flip, and the share of odd ones goes to a half. */
TEST hod_reaches_half_once_the_low_bits_are_random(void) {
    static unsigned char samples[1 << 16];
    unsigned int state = 20250905u;

    for (size_t i = 0; i < sizeof(samples); i++)
        samples[i] = (unsigned char)(128 | (next_random(&state) & 1u));

    struct PixelBuffer pixels = {.samples = samples, .len = sizeof(samples), .width = 256, .height = 256, .channels = 1};

    struct StatResult result;
    ASSERT_EQ(0, stats_hod(&pixels, &result));

    ASSERT(result.applicable);
    ASSERT_IN_RANGE(50.0, result.percent, 2.0);
    PASS();
}

TEST hod_needs_a_neighbour(void) {
    unsigned char samples[4] = {1, 2, 3, 4};

    struct PixelBuffer pixels = {.samples = samples, .len = sizeof(samples), .width = 1, .height = 4, .channels = 1};

    struct StatResult result;
    ASSERT_EQ(0, stats_hod(&pixels, &result));

    ASSERT_FALSE(result.applicable);
    PASS();
}

TEST the_coefficient_histogram_drops_what_is_out_of_range(void) {
    short values[7] = {2, -3, 1000, -1000, STAT_COEFFICIENT_RANGE, -STAT_COEFFICIENT_RANGE, STAT_COEFFICIENT_RANGE + 1};

    struct DctCoefficients coefficients = {.values = values, .count = 7, .width = 8, .height = 8, .components = 1};

    size_t histogram[STAT_COEFFICIENT_LEVELS];
    ASSERT_EQ(0, stats_coefficient_histogram(&coefficients, histogram));

    size_t total = 0;
    for (size_t i = 0; i < STAT_COEFFICIENT_LEVELS; i++)
        total += histogram[i];

    ASSERT_EQ((size_t)4, total);
    ASSERT_EQ((size_t)1, histogram[2 + STAT_COEFFICIENT_ZERO]);
    ASSERT_EQ((size_t)1, histogram[-3 + STAT_COEFFICIENT_ZERO]);
    ASSERT_EQ((size_t)1, histogram[STAT_COEFFICIENT_RANGE + STAT_COEFFICIENT_ZERO]);
    ASSERT_EQ((size_t)1, histogram[-STAT_COEFFICIENT_RANGE + STAT_COEFFICIENT_ZERO]);
    PASS();
}

/* The coefficient pairs run (2k, 2k + 1) the same way the sample pairs do,
 * negative k included, and the pair holding 0 and 1 is not one the codec can
 * reach. Level counts everywhere else means a full payload. */
TEST the_dct_histogram_is_certain_when_every_pair_is_level(void) {
    short values[62 * 16];
    size_t count = 0;

    for (int value = -STAT_COEFFICIENT_RANGE; value < STAT_COEFFICIENT_RANGE; value++) {
        if (value == 0 || value == 1)
            continue;

        for (int i = 0; i < 16; i++)
            values[count++] = (short)value;
    }

    struct DctCoefficients coefficients = {.values = values, .count = count, .width = 8, .height = 8, .components = 1};

    struct StatResult result;
    ASSERT_EQ(0, stats_jpeg_histogram(&coefficients, &result));

    ASSERT(result.applicable);
    ASSERT_IN_RANGE(100.0, result.percent, 0.001);
    ASSERT_IN_RANGE(100.0, result.reference, 0.001);
    ASSERT_EQ((size_t)(STAT_COEFFICIENT_RANGE - 1), result.population);
    PASS();
}

/* What an untouched JPEG looks like: the smaller half of every pair is much
 * the rarer one. */
TEST the_dct_histogram_sees_an_untouched_carrier(void) {
    short values[48 * 62];
    size_t count = 0;

    for (int value = -STAT_COEFFICIENT_RANGE; value < STAT_COEFFICIENT_RANGE; value++) {
        if (value == 0 || value == 1)
            continue;

        /* The even member of each pair carries five times the count. */
        const int repeats = (value % 2 == 0) ? 40 : 8;

        for (int i = 0; i < repeats; i++)
            values[count++] = (short)value;
    }

    struct DctCoefficients coefficients = {.values = values, .count = count, .width = 8, .height = 8, .components = 1};

    struct StatResult result;
    ASSERT_EQ(0, stats_jpeg_histogram(&coefficients, &result));

    ASSERT(result.applicable);
    ASSERT(result.percent < 0.001);
    PASS();
}

TEST every_method_names_itself(void) {
    for (size_t i = 0; i < STAT_METHOD_COUNT; i++) {
        const enum StatMethod method = (enum StatMethod)i;

        ASSERT(strlen(stat_method_name(method)) > 0);
        ASSERT(strlen(stat_method_summary(method)) > 0);
        ASSERT(strlen(stat_method_unit(method)) > 0);
    }

    ASSERT_STR_EQ("unknown", stat_method_name((enum StatMethod)STAT_METHOD_COUNT));
    PASS();
}

TEST a_png_runs_everything_but_the_coefficients(void) {
    char *path = create_test_png(CARRIER_SIZE, CARRIER_SIZE, 3);
    ASSERT(path != NULL);

    struct StatSuite suite;
    ASSERT_EQ(0, stats_analyse(path, &suite));

    ASSERT_EQ(TYPE_PNG_IMAGE, suite.type);
    ASSERT_EQ(CARRIER_SIZE, suite.width);
    ASSERT_EQ(CARRIER_SIZE, suite.height);
    ASSERT_EQ(3, suite.channels);
    ASSERT_EQ((size_t)0, suite.coefficients);

    ASSERT(suite.results[STAT_LSB_RATIO].applicable);
    ASSERT(suite.results[STAT_CHI_SQUARE].applicable);
    ASSERT(suite.results[STAT_HOD].applicable);
    ASSERT_FALSE(suite.results[STAT_JPEG_HISTOGRAM].applicable);

    unlink(path);
    free(path);
    PASS();
}

TEST a_jpeg_runs_the_coefficients_too(void) {
    char *path = create_test_jpg(CARRIER_SIZE, CARRIER_SIZE, CARRIER_QUALITY);
    ASSERT(path != NULL);

    struct StatSuite suite;
    ASSERT_EQ(0, stats_analyse(path, &suite));

    ASSERT_EQ(TYPE_JPEG_IMAGE, suite.type);
    ASSERT(suite.coefficients > 0);

    for (size_t i = 0; i < STAT_METHOD_COUNT; i++) {
        const struct StatResult *result = &suite.results[i];

        if (!result->applicable)
            continue;

        ASSERT(result->percent >= 0.0);
        ASSERT(result->percent <= 100.0);
    }

    unlink(path);
    free(path);
    PASS();
}

TEST refuses_what_it_cannot_read(void) {
    struct StatSuite suite;

    ASSERT_EQ(-1, stats_analyse(NULL, &suite));
    ASSERT_EQ(-1, stats_analyse("/nonexistent/nope.png", &suite));
    ASSERT_EQ(-1, stats_analyse("/nonexistent/nope.png", NULL));
    PASS();
}

/* The whole point of the module: encode into a carrier whose low bits are all
 * clear and watch every pixel method move towards the reference. */
TEST an_encoded_carrier_moves_towards_the_reference(void) {
    char *carrier = create_banded_png();
    ASSERT(carrier != NULL);

    char *output = temp_path();
    ASSERT(output != NULL);

    struct StatSuite before;
    ASSERT_EQ(0, stats_analyse(carrier, &before));

    ASSERT_IN_RANGE(0.0, before.results[STAT_LSB_RATIO].percent, 0.001);
    ASSERT_IN_RANGE(0.0, before.results[STAT_HOD].percent, 0.001);
    ASSERT(before.results[STAT_CHI_SQUARE].applicable);
    ASSERT(before.results[STAT_CHI_SQUARE].percent < 1.0);

    unsigned char *payload = malloc(PAYLOAD_LEN);
    ASSERT(payload != NULL);

    unsigned int state = 7u;
    for (size_t i = 0; i < PAYLOAD_LEN; i++)
        payload[i] = (unsigned char)next_random(&state);

    struct ImageCtx ctx = {
        .source_file = carrier,
        .output_file = output,

        .image_type = TYPE_PNG_IMAGE,
        .codec_type = CODEC_LSB_REPLACEMENT,

        .passphrase = NULL,
    };

    ASSERT_EQ(0, encode_image(&ctx, payload, PAYLOAD_LEN));
    free(payload);

    struct StatSuite after;
    ASSERT_EQ(0, stats_analyse(output, &after));

    /* Nearly every slot is written, so the low bits are nearly all random. */
    ASSERT(after.results[STAT_LSB_RATIO].percent > 40.0);
    ASSERT(after.results[STAT_HOD].percent > 40.0);
    ASSERT(after.results[STAT_CHI_SQUARE].percent > before.results[STAT_CHI_SQUARE].percent);

    unlink(carrier);
    unlink(output);
    free(carrier);
    free(output);
    PASS();
}

SUITE(stats_suite) {
    RUN_TEST(the_sample_histogram_skips_alpha);
    RUN_TEST(the_low_bit_ratio_is_the_share_of_odd_samples);
    RUN_TEST(the_low_bit_ratio_balances_at_half);
    RUN_TEST(chi_square_is_certain_when_every_pair_is_level);
    RUN_TEST(chi_square_sees_through_a_lopsided_histogram);
    RUN_TEST(chi_square_refuses_to_guess_from_one_pair);
    RUN_TEST(differences_are_taken_along_a_row_only);
    RUN_TEST(the_difference_histogram_keeps_the_sign);
    RUN_TEST(hod_bottoms_out_on_a_flat_carrier);
    RUN_TEST(hod_reaches_half_once_the_low_bits_are_random);
    RUN_TEST(hod_needs_a_neighbour);
    RUN_TEST(the_coefficient_histogram_drops_what_is_out_of_range);
    RUN_TEST(the_dct_histogram_is_certain_when_every_pair_is_level);
    RUN_TEST(the_dct_histogram_sees_an_untouched_carrier);
    RUN_TEST(every_method_names_itself);
    RUN_TEST(a_png_runs_everything_but_the_coefficients);
    RUN_TEST(a_jpeg_runs_the_coefficients_too);
    RUN_TEST(refuses_what_it_cannot_read);
    RUN_TEST(an_encoded_carrier_moves_towards_the_reference);
}
