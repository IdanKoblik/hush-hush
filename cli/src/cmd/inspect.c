#include "core/analysis/inspect.h"
#include "command.h"
#include <core/log.h>
#include <flag.h>
#include <stdio.h>
#include <stdlib.h>

int exec(int argc, char *argv[]) {
    const char *target = argv[0];
    if (!target)
        return EXEC_USAGE_ERROR;

    size_t line_limit = 0;
    flag_size_var(&line_limit, "l", 10, "Line limit");

    if (!flag_parse(argc, argv)) {
        flag_print_error(stderr);
        return EXEC_USAGE_ERROR;
    }

    if (line_limit <= 0) {
        ERROR("Line limit cannot be equal or less then 0");
        return EXEC_GENERIC_ERROR;
    }

    struct PixelBuffer pixels;
    if (pixels_load(target, &pixels) != 0)
        return EXEC_GENERIC_ERROR;

    INFO("Image: %dx%d, %d channels", pixels.width, pixels.height, pixels.channels);

    struct LsbStream stream;
    if (inspect_lsb(&pixels, line_limit, &stream) != 0) {
        ERROR("Failed to inspect target lsb");
        pixels_free(&pixels);
        return EXEC_GENERIC_ERROR;
    }

    for (size_t i = 0; i < stream.len; i++) {
        struct InspectRow row;
        inspect_row(stream.bytes[i], &row);

        printf("%s %s %s\n", row.binary, row.hex, row.ascii);
    }

    lsb_stream_free(&stream);
    pixels_free(&pixels);
    return EXEC_OK;
}

static const struct Command inspect_cmd = {.name = "inspect", .description = "Inspect data of an image", .usage = "Usage: inspect <target_file> [-l <line_limit>]\n", .exec = exec};

COMMAND(inspect_cmd);
