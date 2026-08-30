#include "handlers/image.h"
#include "codecs/dct.h"
#include "codecs/lsb.h"
#include "log.h"
#include "structs/image.h"
#include "utils/file.h"
#include <stdio.h>
#include <string.h>

static const Codec *codec_for(enum FileType type) {
    switch (type) {
    case TYPE_PNG_IMAGE:
        return &LsbCodec;
    case TYPE_JPEG_IMAGE:
        return &DctCodec;
    default:
        return NULL;
    }
}

enum FileType detect_image(const char *path) {
    int width;
    int height;
    int channels;

    DEBUG("Probing image: %s", path);
    if (!stbi_info(path, &width, &height, &channels))
        return TYPE_UNKNOWN;

    unsigned char header[12];
    FILE *file = fopen(path, "rb");
    if (!file)
        return TYPE_NOT_FOUND;

    size_t len = fread(header, 1, sizeof(header), file);
    fclose(file);

    /*
     * PNG header (https://en.wikipedia.org/wiki/PNG#File_format)
     *
     * 89 50 4E 47 0D 0A 1A 0A
     */
    if (len >= 8 && memcmp(header, "\x89PNG\r\n\x1a\n", 8) == 0) {
        DEBUG("Detected PNG image: %dx%d, %d channels", width, height, channels);
        return TYPE_PNG_IMAGE;
    }

    /*
     * JPEG header (https://en.wikipedia.org/wiki/JPEG_File_Interchange_Format)
     *
     * FF D8 FF
     */
    if (len >= 3 && memcmp(header, "\xff\xd8\xff", 3) == 0) {
        DEBUG("Detected JPEG image: %dx%d, %d channels", width, height, channels);
        return TYPE_JPEG_IMAGE;
    }

    return TYPE_UNKNOWN;
}

int encode_image(struct ImageCtx *ctx, unsigned char *data, size_t data_len) {
    const Codec *codec = codec_for(ctx->image_type);
    if (!codec) {
        ERROR("Unsupported file type");
        return -1;
    }

    return codec->encode(ctx, data, data_len);
}

int decode_image(struct ImageCtx *ctx, unsigned char **data, size_t *data_len) {
    const Codec *codec = codec_for(ctx->image_type);
    if (!codec) {
        ERROR("Unsupported file type");
        return -1;
    }

    return codec->decode(ctx, data, data_len);
}
