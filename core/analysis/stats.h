#pragma once

#include <core/analysis/dct.h>
#include <core/analysis/inspect.h>
#include <core/fs/file.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STAT_SAMPLE_LEVELS 256
#define STAT_DIFFERENCE_ZERO 255
#define STAT_DIFFERENCE_LEVELS 511

#define STAT_COEFFICIENT_RANGE 10
#define STAT_COEFFICIENT_ZERO STAT_COEFFICIENT_RANGE
#define STAT_COEFFICIENT_LEVELS (2 * STAT_COEFFICIENT_RANGE + 1)

enum StatMethod {
    STAT_LSB_RATIO,
    STAT_CHI_SQUARE,
    STAT_HOD,
    STAT_JPEG_HISTOGRAM,

    STAT_METHOD_COUNT,
};

struct StatResult {
    int applicable;
    double percent;
    double reference;
    size_t population;
};

struct StatSuite {
    enum FileType type;

    int width;
    int height;
    int channels;
    size_t coefficients;

    struct StatResult results[STAT_METHOD_COUNT];
};

const char *stat_method_name(enum StatMethod method);
const char *stat_method_summary(enum StatMethod method);
const char *stat_method_unit(enum StatMethod method);

int stats_sample_histogram(const struct PixelBuffer *pixels, size_t *out);
int stats_difference_histogram(const struct PixelBuffer *pixels, size_t *out);
int stats_coefficient_histogram(const struct DctCoefficients *coefficients, size_t *out);

int stats_lsb_ratio(const struct PixelBuffer *pixels, struct StatResult *out);
int stats_chi_square(const struct PixelBuffer *pixels, struct StatResult *out);
int stats_hod(const struct PixelBuffer *pixels, struct StatResult *out);
int stats_jpeg_histogram(const struct DctCoefficients *coefficients, struct StatResult *out);

int stats_run(enum FileType type, const struct PixelBuffer *pixels, const struct DctCoefficients *coefficients, struct StatSuite *out);
int stats_analyse(const char *target, struct StatSuite *out);

#ifdef __cplusplus
}
#endif
