#pragma once

#include <core/analysis/inspect.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct DctCoefficients {
    short *values;
    size_t count;

    int width;
    int height;
    int components;
};

int dct_load(const char *target, struct DctCoefficients *out);
void dct_free(struct DctCoefficients *coefficients);

struct DctStream {
    unsigned char *bytes;
    size_t len;
};

int inspect_dct(const struct DctCoefficients *coefficients, size_t limit, struct DctStream *out);
void dct_stream_free(struct DctStream *stream);

#ifdef __cplusplus
}
#endif
