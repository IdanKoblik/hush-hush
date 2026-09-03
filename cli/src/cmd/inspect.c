#include "command.h"
#include <core/log.h>
#include <flag.h>
#include <stb_image.h>
#include <stdlib.h>
#include <unistd.h>

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

    int width, height, channels;
    unsigned char *pixels = stbi_load(target, &width, &height, &channels, 0 /* Any */);

    if (!pixels) {
        ERROR("Failed to load the image (%s)", target);
        return -1;
    }

    size_t pixels_len = (size_t)width * height * channels * sizeof(*pixels);
    INFO("Image: %dx%d, %d channels", width, height, channels);
    size_t lines = 0;

    for (size_t i = 0; i + 7 < pixels_len; i += 8) {
        unsigned char byte = 0;

        for (size_t j = 0; j < 8; j++)
            byte |= ((pixels[i + j] & 1) << j);

        char line[64];
        int len = 0;

        // Binary
        for (int bit = 7; bit >= 0; bit--)
            len += snprintf(line + len, sizeof(line) - len, "%d", (byte >> bit) & 1);

        // Hex
        len += snprintf(line + len, sizeof(line) - len, "  0x%02x  ", byte);

        // ASCII
        len += snprintf(line + len, sizeof(line) - len, "%c\n", (byte >= 32 && byte <= 126) ? byte : '.');

        if (isatty(STDOUT_FILENO)) {
            if (lines < line_limit) {
                fputs(line, stdout);
                lines++;
            }
        } else
            fputs(line, stdout);
    }

    INFO("To see the rest of the inspect pipe the command to a text file.");

    stbi_image_free(pixels);
    return EXEC_OK;
}

static const struct Command inspect_cmd = {.name = "inspect", .description = "Inspect data of an image", .usage = "Usage: inspect <target_file> [-l <line_limit>]\n", .exec = exec};

COMMAND(inspect_cmd);
