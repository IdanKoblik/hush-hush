#include "dct.h"

#include "carrier.h"
#include "container.h"
#include "core/handlers/image.h"
#include "core/log.h"

#include <setjmp.h>
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>

#include <jpeglib.h>

struct JpegError {
    struct jpeg_error_mgr mgr;
    jmp_buf escape;
};

struct JpegImage {
    struct jpeg_decompress_struct decoder;
    struct JpegError error;
    jvirt_barray_ptr *arrays;
    FILE *file;
};

struct CoefficientCarrier {
    JCOEF *values;
    size_t count;
    enum CodecType codec;
};

typedef void (*CoefficientVisitor)(JCOEF *coefficient, size_t slot, void *ctx);

static int coefficient_usable(JCOEF value) {
    return value != 0 && value != 1;
}

static void dct_replacement(JCOEF *coefficient, unsigned char bit) {
    *coefficient = (JCOEF)((*coefficient & ~1) | bit);
}

static void dct_matching(JCOEF *coefficient, unsigned char bit) {
    if ((*coefficient & 1) == bit)
        return;

    JCOEF down = (JCOEF)(*coefficient - 1);
    JCOEF up = (JCOEF)(*coefficient + 1);

    if (!coefficient_usable(down))
        *coefficient = up;
    else if (!coefficient_usable(up))
        *coefficient = down;
    else
        *coefficient = randombytes_uniform(2) ? up : down;
}

static unsigned char coefficient_read(const Carrier *carrier, size_t slot) {
    const struct CoefficientCarrier *coefficients = carrier->ctx;

    return (unsigned char)(coefficients->values[slot] & 1);
}

static void coefficient_write(const Carrier *carrier, size_t slot, unsigned char bit) {
    struct CoefficientCarrier *coefficients = carrier->ctx;
    JCOEF *value = &coefficients->values[slot];

    switch (coefficients->codec) {
    case CODEC_LSB_MATCHING: {
        dct_matching(value, bit);
        break;
    }
    default:
        dct_replacement(value, bit);
    }
}

static void jpeg_escape(j_common_ptr info) {
    struct JpegError *error = (struct JpegError *)info->err;
    char message[JMSG_LENGTH_MAX];

    (*info->err->format_message)(info, message);
    ERROR("libjpeg: %s", message);

    longjmp(error->escape, 1);
}

static int jpeg_image_open(struct JpegImage *image, const char *path) {
    image->arrays = NULL;
    image->file = fopen(path, "rb");

    if (!image->file) {
        ERROR("Failed to load the image (%s)", path);
        return -1;
    }

    image->decoder.err = jpeg_std_error(&image->error.mgr);
    image->error.mgr.error_exit = jpeg_escape;

    if (setjmp(image->error.escape)) {
        jpeg_destroy_decompress(&image->decoder);
        fclose(image->file);
        image->file = NULL;
        return -1;
    }

    jpeg_create_decompress(&image->decoder);
    jpeg_stdio_src(&image->decoder, image->file);
    jpeg_read_header(&image->decoder, TRUE);
    image->arrays = jpeg_read_coefficients(&image->decoder);

    return 0;
}

static void jpeg_image_close(struct JpegImage *image) {
    if (!image->file)
        return;

    if (!setjmp(image->error.escape))
        jpeg_finish_decompress(&image->decoder);

    jpeg_destroy_decompress(&image->decoder);
    fclose(image->file);
    image->file = NULL;
}

static size_t walk_coefficients(struct JpegImage *image, CoefficientVisitor visit, void *ctx) {
    j_decompress_ptr decoder = &image->decoder;
    size_t slot = 0;

    // O(num of blocks). Because DCTSIZE2 is a compile-time constant.
    for (int component = 0; component < decoder->num_components; component++) {
        jpeg_component_info *info = &decoder->comp_info[component];

        for (JDIMENSION row = 0; row < info->height_in_blocks; row++) {
            JBLOCKARRAY blocks = (*decoder->mem->access_virt_barray)((j_common_ptr)decoder, image->arrays[component], row, 1, TRUE);

            for (JDIMENSION block = 0; block < info->width_in_blocks; block++) {
                JCOEFPTR coefficients = blocks[0][block];

                for (int index = 1; index < DCTSIZE2; index++) {
                    if (!coefficient_usable(coefficients[index]))
                        continue;

                    if (visit)
                        visit(&coefficients[index], slot, ctx);

                    slot++;
                }
            }
        }
    }

    return slot;
}

static void coefficient_collect(JCOEF *coefficient, size_t slot, void *ctx) {
    ((JCOEF *)ctx)[slot] = *coefficient;
}

static void coefficient_apply(JCOEF *coefficient, size_t slot, void *ctx) {
    *coefficient = ((const JCOEF *)ctx)[slot];
}

static int coefficients_load(struct JpegImage *image, struct CoefficientCarrier *coefficients) {
    if (setjmp(image->error.escape))
        return -1;

    coefficients->count = walk_coefficients(image, NULL, NULL);
    if (coefficients->count == 0) {
        ERROR("The image carries no usable DCT coefficients");
        return -1;
    }

    coefficients->values = calloc(coefficients->count, sizeof(*coefficients->values));
    if (!coefficients->values) {
        ERROR("Failed to allocate the coefficient buffer");
        return -1;
    }

    walk_coefficients(image, coefficient_collect, coefficients->values);

    return 0;
}

static int coefficients_store(struct JpegImage *image, const struct CoefficientCarrier *coefficients) {
    if (setjmp(image->error.escape))
        return -1;

    walk_coefficients(image, coefficient_apply, coefficients->values);

    return 0;
}

static int jpeg_image_write(struct JpegImage *image, const char *path) {
    struct jpeg_compress_struct encoder;
    FILE *const file = fopen(path, "wb");

    if (!file) {
        ERROR("Failed to write into the targeted file");
        return -1;
    }

    encoder.err = &image->error.mgr;

    if (setjmp(image->error.escape)) {
        jpeg_destroy_compress(&encoder);
        fclose(file);
        return -1;
    }

    jpeg_create_compress(&encoder);
    jpeg_stdio_dest(&encoder, file);
    jpeg_copy_critical_parameters(&image->decoder, &encoder);

    if (image->decoder.progressive_mode)
        jpeg_simple_progression(&encoder);

    jpeg_write_coefficients(&encoder, image->arrays);
    jpeg_finish_compress(&encoder);
    jpeg_destroy_compress(&encoder);

    return fclose(file) == 0 ? 0 : -1;
}

static int carrier_open(struct ImageCtx *img, struct JpegImage *image, struct CoefficientCarrier *coefficients, Carrier *carrier) {
    coefficients->values = NULL;
    coefficients->count = 0;
    coefficients->codec = img->codec_type;

    INFO("Loading image: %s", img->source_file);
    if (jpeg_image_open(image, img->source_file) < 0)
        return -1;

    img->width = (int)image->decoder.image_width;
    img->height = (int)image->decoder.image_height;
    img->channels = image->decoder.num_components;

    if (coefficients_load(image, coefficients) < 0)
        return -1;

    carrier->ctx = coefficients;
    carrier->slots = coefficients->count;
    carrier->read = coefficient_read;
    carrier->write = coefficient_write;

    INFO("Image: %dx%d, %d channels", img->width, img->height, img->channels);

    return 0;
}

static void carrier_close(struct JpegImage *image, struct CoefficientCarrier *coefficients) {
    free(coefficients->values);
    coefficients->values = NULL;

    jpeg_image_close(image);
}

static int encode(void *ctx, const unsigned char *data, size_t data_len) {
    struct ImageCtx *img = ctx;

    struct JpegImage image;
    struct CoefficientCarrier coefficients;
    Carrier carrier;

    int status = carrier_open(img, &image, &coefficients, &carrier);

    if (status == 0)
        status = container_embed(&carrier, img->codec_type, img->passphrase, data, data_len);

    if (status == 0)
        status = coefficients_store(&image, &coefficients);

    if (status == 0) {
        DEBUG("Writing output image: %s", img->output_file);
        status = jpeg_image_write(&image, img->output_file);
    }

    carrier_close(&image, &coefficients);
    if (status < 0)
        return -1;

    INFO("Encoded successfully -> %s", img->output_file);

    return 0;
}

static int decode(void *ctx, unsigned char **data, size_t *data_len) {
    struct ImageCtx *img = ctx;

    struct JpegImage image;
    struct CoefficientCarrier coefficients;
    Carrier carrier;

    int status = carrier_open(img, &image, &coefficients, &carrier);

    if (status == 0)
        status = container_extract(&carrier, img->passphrase, &img->codec_type, data, data_len);

    carrier_close(&image, &coefficients);
    if (status < 0)
        return -1;

    INFO("Decoded %zu bytes from %s", *data_len, img->source_file);

    return 0;
}

// GOD I HAVE JPEG
const Codec DctCodec = {.encode = encode, .decode = decode};
