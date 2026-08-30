#pragma once

#include "codecs/codec.h"
#include "utils/file.h"

struct ImageCtx {
    int height;
    int width;
    int channels;

    const char *source_file;
    const char *output_file;

    enum CodecType codec_type;
    enum FileType image_type;

    const char *passphrase;
};