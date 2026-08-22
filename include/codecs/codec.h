#ifndef CODEC_H_
#define CODEC_H_

#include <stddef.h>

enum CodecType {
    CODEC_LSB_MATCHING,
    CODEC_LSB_REPLACEMENT,
    CODEC_UNKNOWN
};

enum CodecType str_to_codec(const char *str);

typedef struct Codec {
    int (*encode)(void *ctx, const unsigned char *data, size_t data_len);
    int (*decode)(void *ctx);
} Codec;

#endif // CODEC_H_
