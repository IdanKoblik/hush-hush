/*
 * stb_image and stb_image_write are header-only: exactly one translation unit
 * has to pull in their implementations. This is it.
 */
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"
