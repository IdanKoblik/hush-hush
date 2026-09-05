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
