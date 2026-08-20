#ifndef IMAGE_H_
#define IMAGE_H_

#include <stddef.h>
#include "stb_image.h"

enum FileType detect_image(const char *path);

int encode_image(const char *target, enum FileType type, const char *output, unsigned char *data, size_t data_len);

#endif // IMAGE_H_
