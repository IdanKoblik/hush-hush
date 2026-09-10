#pragma once

#include <stddef.h>
#include "../codecs/codec.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PixelBuffer {
    unsigned char *samples;
    size_t len;

    int width;
    int height;
    int channels;

    enum CodecType codec;
};

int pixels_load(const char *target, struct PixelBuffer *out);
void pixels_free(struct PixelBuffer *pixels);

size_t pixel_color_channels(int channels);
size_t pixel_slot_to_sample(size_t slot, size_t colors, size_t channels);

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
