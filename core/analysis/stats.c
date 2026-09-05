#include "stats.h"
#include "core/log.h"

#include <math.h>
#include <string.h>

#define MIN_EXPECTED 5.0

#define SAMPLE_PAIRS (STAT_SAMPLE_LEVELS / 2)
#define COEFFICIENT_PAIRS (STAT_COEFFICIENT_RANGE - 1)

#define GAMMA_ITERATIONS 300
#define GAMMA_EPSILON 3.0e-14
#define GAMMA_TINY 1.0e-300

static double gamma_series(double a, double x) {
    double term = 1.0 / a;
    double sum = term;
    double ap = a;

    for (int i = 0; i < GAMMA_ITERATIONS; i++) {
        ap += 1.0;
        term *= x / ap;
        sum += term;

        if (fabs(term) < fabs(sum) * GAMMA_EPSILON)
            break;
    }

    return sum * exp(-x + a * log(x) - lgamma(a));
}

static double gamma_fraction(double a, double x) {
    double b = x + 1.0 - a;
    double c = 1.0 / GAMMA_TINY;
    double d = 1.0 / b;
    double h = d;

    for (int i = 1; i <= GAMMA_ITERATIONS; i++) {
        const double an = -(double)i * ((double)i - a);

        b += 2.0;

        d = an * d + b;
        if (fabs(d) < GAMMA_TINY)
            d = GAMMA_TINY;

        c = b + an / c;
        if (fabs(c) < GAMMA_TINY)
            c = GAMMA_TINY;

        d = 1.0 / d;

        const double delta = d * c;
        h *= delta;

        if (fabs(delta - 1.0) < GAMMA_EPSILON)
            break;
    }

    return exp(-x + a * log(x) - lgamma(a)) * h;
}

static double gamma_q(double a, double x) {
    if (x <= 0.0 || a <= 0.0)
        return 1.0;

    if (x < a + 1.0)
        return 1.0 - gamma_series(a, x);

    return gamma_fraction(a, x);
}

static double pov_probability(const size_t *left, const size_t *right, size_t pairs, size_t *tested) {
    double chi = 0.0;
    size_t used = 0;

    for (size_t i = 0; i < pairs; i++) {
        const double expected = ((double)left[i] + (double)right[i]) / 2.0;
        if (expected < MIN_EXPECTED)
            continue;

        const double deviation = (double)left[i] - expected;

        chi += deviation * deviation / expected;
        used++;
    }

    if (tested)
        *tested = used;

    if (used < 2)
        return -1.0;

    return gamma_q((double)(used - 1) / 2.0, chi / 2.0);
}

static double method_reference(enum StatMethod method) {
    switch (method) {
    case STAT_CHI_SQUARE:
    case STAT_JPEG_HISTOGRAM:
        return 100.0;
    default:
        return 50.0;
    }
}

static void result_reset(struct StatResult *out, enum StatMethod method) {
    out->applicable = 0;
    out->percent = 0.0;
    out->reference = method_reference(method);
    out->population = 0;
}

const char *stat_method_name(enum StatMethod method) {
    switch (method) {
    case STAT_LSB_RATIO:
        return "Low bit ratio";
    case STAT_CHI_SQUARE:
        return "Chi-square (PoV)";
    case STAT_HOD:
        return "Histogram of differences";
    case STAT_JPEG_HISTOGRAM:
        return "DCT histogram (PoV)";
    default:
        return "unknown";
    }
}

const char *stat_method_summary(enum StatMethod method) {
    switch (method) {
    case STAT_LSB_RATIO:
        return "The share of colour samples whose low bit is set, n1 / (n0 + n1). Random payload bits balance it at 50 %.";
    case STAT_CHI_SQUARE:
        return "How well the sample histogram matches the flattened pairs that LSB replacement leaves behind, as a probability. A carrier with nothing in it sits near 0 %.";
    case STAT_HOD:
        return "The share of differences between neighbouring samples that are odd. Correlated neighbours hold a photograph below 50 %; random low bits pull it to 50 %.";
    case STAT_JPEG_HISTOGRAM:
        return "The same pairs of values test over the quantised DCT coefficients, which is where a JPEG carrier hides its bits.";
    default:
        return "";
    }
}

const char *stat_method_unit(enum StatMethod method) {
    switch (method) {
    case STAT_LSB_RATIO:
        return "colour samples";
    case STAT_CHI_SQUARE:
        return "pairs of values";
    case STAT_HOD:
        return "neighbour pairs";
    case STAT_JPEG_HISTOGRAM:
        return "pairs of values";
    default:
        return "";
    }
}

int stats_sample_histogram(const struct PixelBuffer *pixels, size_t *out) {
    if (!pixels || !pixels->samples || !out)
        return -1;

    const size_t channels = (size_t)pixels->channels;
    const size_t colors = pixel_color_channels(pixels->channels);
    if (colors == 0)
        return -1;

    memset(out, 0, STAT_SAMPLE_LEVELS * sizeof(*out));

    const size_t slots = (size_t)pixels->width * (size_t)pixels->height * colors;

    for (size_t slot = 0; slot < slots; slot++) {
        const size_t sample = pixel_slot_to_sample(slot, colors, channels);
        if (sample >= pixels->len)
            break;

        out[pixels->samples[sample]]++;
    }

    return 0;
}

int stats_difference_histogram(const struct PixelBuffer *pixels, size_t *out) {
    if (!pixels || !pixels->samples || !out)
        return -1;

    const size_t channels = (size_t)pixels->channels;
    const size_t colors = pixel_color_channels(pixels->channels);
    if (colors == 0)
        return -1;

    memset(out, 0, STAT_DIFFERENCE_LEVELS * sizeof(*out));

    for (size_t y = 0; y < (size_t)pixels->height; y++) {
        for (size_t x = 0; x + 1 < (size_t)pixels->width; x++) {
            for (size_t color = 0; color < colors; color++) {
                const size_t here = (y * (size_t)pixels->width + x) * channels + color;
                const size_t next = here + channels;

                if (next >= pixels->len)
                    break;

                const int difference = (int)pixels->samples[next] - (int)pixels->samples[here];

                out[difference + STAT_DIFFERENCE_ZERO]++;
            }
        }
    }

    return 0;
}

int stats_coefficient_histogram(const struct DctCoefficients *coefficients, size_t *out) {
    if (!coefficients || !coefficients->values || !out)
        return -1;

    memset(out, 0, STAT_COEFFICIENT_LEVELS * sizeof(*out));

    for (size_t i = 0; i < coefficients->count; i++) {
        const int value = coefficients->values[i];

        if (value < -STAT_COEFFICIENT_RANGE || value > STAT_COEFFICIENT_RANGE)
            continue;

        out[value + STAT_COEFFICIENT_ZERO]++;
    }

    return 0;
}

int stats_lsb_ratio(const struct PixelBuffer *pixels, struct StatResult *out) {
    if (!out)
        return -1;

    result_reset(out, STAT_LSB_RATIO);

    size_t histogram[STAT_SAMPLE_LEVELS];
    if (stats_sample_histogram(pixels, histogram) != 0)
        return -1;

    size_t zeros = 0;
    size_t ones = 0;

    for (size_t value = 0; value < STAT_SAMPLE_LEVELS; value++) {
        if (value & 1)
            ones += histogram[value];
        else
            zeros += histogram[value];
    }

    const size_t total = zeros + ones;
    if (total == 0)
        return 0;

    out->applicable = 1;
    out->percent = 100.0 * (double)ones / (double)total;
    out->population = total;

    DEBUG("Low bit ratio: %zu set, %zu clear", ones, zeros);
    return 0;
}

int stats_chi_square(const struct PixelBuffer *pixels, struct StatResult *out) {
    if (!out)
        return -1;

    result_reset(out, STAT_CHI_SQUARE);

    size_t histogram[STAT_SAMPLE_LEVELS];
    if (stats_sample_histogram(pixels, histogram) != 0)
        return -1;

    size_t left[SAMPLE_PAIRS];
    size_t right[SAMPLE_PAIRS];

    for (size_t i = 0; i < SAMPLE_PAIRS; i++) {
        left[i] = histogram[i * 2];
        right[i] = histogram[i * 2 + 1];
    }

    size_t tested = 0;
    const double probability = pov_probability(left, right, SAMPLE_PAIRS, &tested);

    out->population = tested;
    if (probability < 0.0)
        return 0;

    out->applicable = 1;
    out->percent = probability * 100.0;

    DEBUG("Chi-square over %zu pairs of values: %.4f", tested, probability);
    return 0;
}

int stats_hod(const struct PixelBuffer *pixels, struct StatResult *out) {
    if (!out)
        return -1;

    result_reset(out, STAT_HOD);

    size_t histogram[STAT_DIFFERENCE_LEVELS];
    if (stats_difference_histogram(pixels, histogram) != 0)
        return -1;

    size_t even = 0;
    size_t odd = 0;

    for (size_t i = 0; i < STAT_DIFFERENCE_LEVELS; i++) {
        const int difference = (int)i - STAT_DIFFERENCE_ZERO;

        if ((difference & 1) != 0)
            odd += histogram[i];
        else
            even += histogram[i];
    }

    const size_t total = even + odd;
    if (total == 0)
        return 0;

    out->applicable = 1;
    out->percent = 100.0 * (double)odd / (double)total;
    out->population = total;

    DEBUG("Histogram of differences: %zu odd, %zu even", odd, even);
    return 0;
}

int stats_jpeg_histogram(const struct DctCoefficients *coefficients, struct StatResult *out) {
    if (!out)
        return -1;

    result_reset(out, STAT_JPEG_HISTOGRAM);

    size_t histogram[STAT_COEFFICIENT_LEVELS];
    if (stats_coefficient_histogram(coefficients, histogram) != 0)
        return -1;

    size_t left[COEFFICIENT_PAIRS];
    size_t right[COEFFICIENT_PAIRS];
    size_t pairs = 0;

    for (int k = -STAT_COEFFICIENT_RANGE / 2; k < STAT_COEFFICIENT_RANGE / 2; k++) {
        if (k == 0)
            continue;

        left[pairs] = histogram[k * 2 + STAT_COEFFICIENT_ZERO];
        right[pairs] = histogram[k * 2 + 1 + STAT_COEFFICIENT_ZERO];
        pairs++;
    }

    size_t tested = 0;
    const double probability = pov_probability(left, right, pairs, &tested);

    out->population = tested;

    if (probability < 0.0)
        return 0;

    out->applicable = 1;
    out->percent = probability * 100.0;

    DEBUG("DCT chi-square over %zu pairs of values: %.4f", tested, probability);
    return 0;
}

int stats_run(enum FileType type, const struct PixelBuffer *pixels, const struct DctCoefficients *coefficients, struct StatSuite *out) {
    if (!pixels || !pixels->samples || !out)
        return -1;

    memset(out, 0, sizeof(*out));

    out->type = type;
    out->width = pixels->width;
    out->height = pixels->height;
    out->channels = pixels->channels;
    out->coefficients = coefficients ? coefficients->count : 0;

    if (stats_lsb_ratio(pixels, &out->results[STAT_LSB_RATIO]) != 0)
        return -1;

    if (stats_chi_square(pixels, &out->results[STAT_CHI_SQUARE]) != 0)
        return -1;

    if (stats_hod(pixels, &out->results[STAT_HOD]) != 0)
        return -1;

    result_reset(&out->results[STAT_JPEG_HISTOGRAM], STAT_JPEG_HISTOGRAM);
    if (coefficients && stats_jpeg_histogram(coefficients, &out->results[STAT_JPEG_HISTOGRAM]) != 0)
        return -1;

    return 0;
}

int stats_analyse(const char *target, struct StatSuite *out) {
    if (!target || !out)
        return -1;

    const enum FileType type = get_file_type(target);
    if (!is_image_file(type)) {
        ERROR("%s is not a PNG or a JPEG", target);
        return -1;
    }

    struct PixelBuffer pixels;
    if (pixels_load(target, &pixels) != 0)
        return -1;

    struct DctCoefficients coefficients;
    const int loaded = type == TYPE_JPEG_IMAGE && dct_load(target, &coefficients) == 0;

    const int status = stats_run(type, &pixels, loaded ? &coefficients : NULL, out);

    if (loaded)
        dct_free(&coefficients);

    pixels_free(&pixels);
    return status;
}
