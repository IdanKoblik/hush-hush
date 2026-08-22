#ifndef IMAGE_H_
#define IMAGE_H_

#include "stb.h"
#include "structs/image.h"
#include <stddef.h>

enum FileType detect_image(const char *path);

int encode_image(struct ImageCtx *ctx, unsigned char *data, size_t data_len);

#endif // IMAGE_H_
