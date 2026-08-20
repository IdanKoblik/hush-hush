#ifndef STRUCT_IMAGE_H_
#define STRUCT_IMAGE_H_

enum ImageChannel {
    GRAYSCALE = 1,
    GRAYSCALE_ALPHA = 2,
    RGB = 3,
    RGBA = 4,
};

struct PngCtx {
    int height;
    int width;
    int channels;
};

#endif // STRUCT_IMAGE_H_
