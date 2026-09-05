#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum CodecType { CODEC_LSB_MATCHING, CODEC_LSB_REPLACEMENT, CODEC_DCT, CODEC_UNKNOWN };

enum CodecType str_to_codec(const char *str);

typedef struct Codec {
    int (*encode)(void *ctx, const unsigned char *data, size_t data_len);
    int (*decode)(void *ctx, unsigned char **data, size_t *data_len);
} Codec;

#ifdef __cplusplus
}
#endif
