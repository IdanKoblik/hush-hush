#include "inspect.h"
#include "core/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stb_image.h>

static size_t color_channels(int channels) {
    return (channels == 2 || channels == 4) ? (size_t)channels - 1 : (size_t)channels;
}

static size_t slot_to_sample(size_t slot, size_t colors, size_t channels) {
    if (colors == channels)
        return slot;

    return (slot / colors) * channels + (slot % colors);
}

int pixels_load(const char *target, struct PixelBuffer *out) {
    if (!target || !out)
        return -1;

    int width = 0, height = 0, channels = 0;
    unsigned char *samples = stbi_load(target, &width, &height, &channels, 0 /* ANY */);

    if (!samples) {
        ERROR("Failed to load the image (%s)", target);
        return -1;
    }

    out->samples = samples;
    out->len = (size_t)width * (size_t)height * (size_t)channels;
    out->width = width;
    out->height = height;
    out->channels = channels;

    DEBUG("Inspecting %dx%d, %d channels", width, height, channels);
    return 0;
}

void pixels_free(struct PixelBuffer *pixels) {
    if (!pixels)
        return;

    stbi_image_free(pixels->samples);
    memset(pixels, 0, sizeof(*pixels));
}

int inspect_lsb(const struct PixelBuffer *pixels, size_t limit, struct LsbStream *out) {
    if (!pixels || !pixels->samples || !out)
        return -1;

    size_t channels = (size_t)pixels->channels;
    size_t colors = color_channels(pixels->channels);
    if (colors == 0)
        return -1;

    size_t slots = (size_t)pixels->width * (size_t)pixels->height * colors;
    size_t count = slots / 8;

    if (limit != NO_LIMIT && limit < count)
        count = limit;

    unsigned char *bytes = calloc(count > 0 ? count : 1, sizeof(*bytes));
    if (!bytes)
        return -1;

    for (size_t i = 0; i < count; i++) {
        unsigned char byte = 0;

        for (size_t bit = 0; bit < 8; bit++) {
            size_t sample = slot_to_sample(i * 8 + bit, colors, channels);
            if (sample >= pixels->len)
                break;

            byte |= (unsigned char)((pixels->samples[sample] & 1) << bit);
        }

        bytes[i] = byte;
    }

    out->bytes = bytes;
    out->len = count;
    return 0;
}

void lsb_stream_free(struct LsbStream *stream) {
    if (!stream)
        return;

    free(stream->bytes);
    stream->bytes = NULL;
    stream->len = 0;
}

void inspect_row(unsigned char byte, struct InspectRow *out) {
    if (!out)
        return;

    for (int bit = 7; bit >= 0; bit--)
        out->binary[7 - bit] = (char)(((byte >> bit) & 1) + '0');

    out->binary[8] = '\0';

    snprintf(out->hex, sizeof(out->hex), "0x%02X", byte);

    out->ascii[0] = (byte >= 32 && byte <= 126) ? (char)byte : '.';
    out->ascii[1] = '\0';
}

enum ByteClass classify_byte(unsigned char byte) {
    if (byte == 0x00)
        return BYTE_ZERO;

    if (byte == 0xFF)
        return BYTE_FILLED;

    if (byte == ' ' || byte == '\t' || byte == '\n' || byte == '\r')
        return BYTE_WHITESPACE;

    if (byte >= 0x20 && byte <= 0x7E)
        return BYTE_PRINTABLE;

    if (byte < 0x20 || byte == 0x7F)
        return BYTE_CONTROL;

    return BYTE_OTHER;
}
