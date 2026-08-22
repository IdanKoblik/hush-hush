#include "codecs/lsb.h"
#include "log.h"
#include "stb.h"
#include "structs/image.h"

static void lsb_replacement(unsigned char *pixels, const unsigned char *data, size_t data_bits) {
    for (size_t i = 0; i < data_bits; i++) {
        size_t byte_index = i / 8;
        size_t bit_index = i % 8;

        unsigned char bit = (data[byte_index] >> bit_index) & 1;
        pixels[i] = (pixels[i] & LSB_FILTER) | bit;
    }
}

static void lsb_matching(unsigned char *pixels, const unsigned char *data, size_t data_bits) {
    for (size_t i = 0; i < data_bits; i++) {
        size_t byte_index = i / 8;
        size_t bit_index = i % 8;

        unsigned char bit = (data[byte_index] >> bit_index) & 1;
        unsigned char pixel = pixels[i];

        if ((pixel & 1) != bit) {
            switch (pixel) {
            case 0: {
                pixel = 1;
                break;
            }
            case 255: {
                pixel = 254;
                break;
            }
            default: pixel += (rand() & 1) ? 1 : -1;
            }
        
            pixels[i] = pixel;
        }
    }
}

static int encode(void *ctx, const unsigned char *data, size_t data_len) {
    struct ImageCtx *img = ctx;
    
    INFO("Loading image: %s", img->source_file);
    unsigned char *pixels = stbi_load(
        img->source_file,
        &img->width,
        &img->height,
        &img->channels,
        0 // Any
    );

    if (!pixels) {
        ERROR("Failed to load the image (%s)", img->source_file);
        return -1;
    }

    size_t pixels_len = (size_t)img->width * img->height * img->channels * sizeof(*pixels);
    size_t capacity_bits = pixels_len;
    size_t capacity_bytes = capacity_bits / 8;

    INFO("Image: %dx%d, %d channels", img->width, img->height, img->channels);
    INFO("Capacity: %zu bytes", capacity_bytes);
    INFO("Data to encode: %zu bytes", data_len);

    size_t data_bits = data_len * 8;
    if (pixels_len < data_bits) {
        ERROR("Image size is not big enough to embed the targeted data into it.");
        stbi_image_free(pixels);
        return -1;
    }
   
    DEBUG("Embedding %zu bits into pixel data", data_bits);

    switch (img->codec_type) {
    case CODEC_LSB_REPLACEMENT: {
        lsb_replacement(pixels, data, data_bits);
        break;
    }
    case CODEC_LSB_MATCHING: {
        lsb_matching(pixels, data, data_bits);
        break;
    }
    }

    DEBUG("Writing output image: %s", img->output_file);
    if (stbi_write_png(img->output_file, img->width, img->height, img->channels, pixels, img->width * img->channels) < 0) {
        stbi_image_free(pixels);
        ERROR("Failed to write into the targeted file");
        return -1;
    }

    stbi_image_free(pixels);
    INFO("Encoded successfully -> %s", img->output_file);

    return 0;
}

static int decode(void *ctx) {
    ERROR("TODO IMPL");
    return -1;
}

const Codec LsbCodec = {.encode = encode, .decode = decode};
