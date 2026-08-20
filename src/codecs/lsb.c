#include "codecs/lsb.h"
#include "log.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "structs/image.h"

#define LSB_FILTER 0XFE

static int encode(const char *path, const char *output, void *ctx, const unsigned char *data, size_t data_len) {
    struct PngCtx *png = ctx;

    INFO("Loading image: %s", path);
    unsigned char *pixels = stbi_load(
        path,
        &png->width,
        &png->height,
        &png->channels,
        0 // Any
    );

    if (!pixels) {
        ERROR("Failed to load the image (%s)", path);
        return -1;
    }

    size_t pixels_len = (size_t)png->width * png->height * png->channels * sizeof(*pixels);
    size_t capacity_bits = pixels_len;
    size_t capacity_bytes = capacity_bits / 8;

    INFO("Image: %dx%d, %d channels", png->width, png->height, png->channels);
    INFO("Capacity: %zu bytes", capacity_bytes);
    INFO("Data to encode: %zu bytes", data_len);

    size_t data_bits = data_len * 8;
    if (pixels_len < data_bits) {
        ERROR("Image size is not big enough to embed the targeted data into it.");
        stbi_image_free(pixels);
        return -1;
    }

    DEBUG("Embedding %zu bits into pixel data", data_bits);
    for (size_t i = 0; i < data_bits; i++) {
        size_t byte_index = i / 8;
        size_t bit_index = i % 8;

        unsigned char bit = (data[byte_index] >> bit_index) & 1;
        pixels[i] = (pixels[i] & LSB_FILTER) | bit;
    }

    DEBUG("Writing output image: %s", output);
    if (stbi_write_png(output, png->width, png->height, png->channels, pixels, png->width * png->channels) < 0) {
        stbi_image_free(pixels);
        ERROR("Failed to write into the targeted file");
        return -1;
    }

    stbi_image_free(pixels);
    INFO("Encoded successfully -> %s", output);
    return 0;
}

static int decode(const char *path, void *ctx) {
    ERROR("TODO IMPL");
    return -1;
}

const Codec LsbCodec = {.encode = encode, .decode = decode};
