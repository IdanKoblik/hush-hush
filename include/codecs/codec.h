#ifndef CODEC_H_
#define CODEC_H_

#include <stddef.h>

typedef struct Codec {
    int (*encode)(const char *path, const char *output, void *ctx, const unsigned char *data, size_t data_len);
    int (*decode)(const char *path, void *ctx);
} Codec;

#endif // CODEC_H_
