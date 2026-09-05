#include "dct.h"
#include "core/handlers/jpeg.h"
#include "core/log.h"

#include <stdlib.h>
#include <string.h>

static void collect(JCOEF *coefficient, size_t slot, void *ctx) {
    ((short *)ctx)[slot] = (short)*coefficient;
}

int dct_load(const char *target, struct DctCoefficients *out) {
    if (!target || !out)
        return -1;

    struct JpegImage image;
    if (jpeg_image_open(&image, target) < 0)
        return -1;

    const size_t count = jpeg_walk_coefficients(&image, NULL, NULL);
    if (count == 0) {
        ERROR("The image carries no usable DCT coefficients");
        jpeg_image_close(&image);
        return -1;
    }

    short *values = calloc(count, sizeof(*values));
    if (!values) {
        jpeg_image_close(&image);
        return -1;
    }

    if (jpeg_walk_coefficients(&image, collect, values) == 0) {
        free(values);
        jpeg_image_close(&image);
        return -1;
    }

    out->values = values;
    out->count = count;
    out->width = (int)image.decoder.image_width;
    out->height = (int)image.decoder.image_height;
    out->components = image.decoder.num_components;

    DEBUG("Inspecting %dx%d, %d components, %zu coefficients", out->width, out->height, out->components, count);

    jpeg_image_close(&image);
    return 0;
}

void dct_free(struct DctCoefficients *coefficients) {
    if (!coefficients)
        return;

    free(coefficients->values);
    memset(coefficients, 0, sizeof(*coefficients));
}

int inspect_dct(const struct DctCoefficients *coefficients, size_t limit, struct DctStream *out) {
    if (!coefficients || !coefficients->values || !out)
        return -1;

    size_t count = coefficients->count / 8;

    if (limit != NO_LIMIT && limit < count)
        count = limit;

    unsigned char *bytes = calloc(count > 0 ? count : 1, sizeof(*bytes));
    if (!bytes)
        return -1;

    for (size_t i = 0; i < count; i++) {
        unsigned char byte = 0;

        for (size_t bit = 0; bit < 8; bit++)
            byte |= (unsigned char)((coefficients->values[i * 8 + bit] & 1) << bit);

        bytes[i] = byte;
    }

    out->bytes = bytes;
    out->len = count;
    return 0;
}

void dct_stream_free(struct DctStream *stream) {
    if (!stream)
        return;

    free(stream->bytes);
    stream->bytes = NULL;
    stream->len = 0;
}
