#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NO_LIMIT 0

struct PixelBuffer {
    unsigned char *samples;
    size_t len;

    int width;
    int height;
    int channels;
};

int pixels_load(const char *target, struct PixelBuffer *out);
void pixels_free(struct PixelBuffer *pixels);

/* How many of a carrier's channels hold colour. The codec never writes into an
 * alpha channel, so nothing that reads the low bits back may look at one
 * either, and everything that walks a carrier goes through these two. */
size_t pixel_color_channels(int channels);

/* Turns a carrier slot, counted over colour samples alone, into the index of
 * the sample holding it. */
size_t pixel_slot_to_sample(size_t slot, size_t colors, size_t channels);

struct LsbStream {
    unsigned char *bytes;
    size_t len;
};

int inspect_lsb(const struct PixelBuffer *pixels, size_t limit, struct LsbStream *out);
void lsb_stream_free(struct LsbStream *stream);

struct InspectRow {
    char binary[9];
    char hex[5];
    char ascii[2];
};

void inspect_row(unsigned char byte, struct InspectRow *out);

enum ByteClass {
    BYTE_ZERO,
    BYTE_FILLED,
    BYTE_WHITESPACE,
    BYTE_PRINTABLE,
    BYTE_CONTROL,
    BYTE_OTHER,
};

enum ByteClass classify_byte(unsigned char byte);

#ifdef __cplusplus
}
#endif
