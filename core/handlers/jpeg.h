#pragma once

#include <setjmp.h>
#include <stddef.h>
#include <stdio.h>

#include <jpeglib.h>

#ifdef __cplusplus
extern "C" {
#endif

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

typedef void (*CoefficientVisitor)(JCOEF *coefficient, size_t slot, void *ctx);

/* A coefficient of 0 or 1 is left untouched, so it carries nothing. Both the
 * codec that writes the low bits and the view that reads them back have to
 * agree on this, which is why it lives here. */
int jpeg_coefficient_usable(JCOEF value);

int jpeg_image_open(struct JpegImage *image, const char *path);
void jpeg_image_close(struct JpegImage *image);
int jpeg_image_write(struct JpegImage *image, const char *path);

/* Visits every usable coefficient in carrier order and returns how many there
 * were. Pass a NULL visitor to count them. Returns 0 if libjpeg failed. */
size_t jpeg_walk_coefficients(struct JpegImage *image, CoefficientVisitor visit, void *ctx);

#ifdef __cplusplus
}
#endif
