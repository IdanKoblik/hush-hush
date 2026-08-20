#ifndef HELPERS_H_
#define HELPERS_H_

#include "stb_image_write.h"
#include <stdio.h>
#include <stdlib.h>

static inline char *create_test_png(int width, int height, int channels) {
    char path[] = "/tmp/test_XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0)
        return NULL;
    close(fd);

    unsigned char *pixels = calloc((size_t)(width * height * channels), 1);
    if (!pixels) {
        unlink(path);
        return NULL;
    }

    for (int i = 0; i < width * height * channels; i++)
        pixels[i] = (unsigned char)(i % 256);

    int stride = width * channels;
    if (stbi_write_png(path, width, height, channels, pixels, stride) == 0) {
        free(pixels);
        unlink(path);
        return NULL;
    }

    free(pixels);
    return strdup(path);
}

#endif // HELPERS_H_
