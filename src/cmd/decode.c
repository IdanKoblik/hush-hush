#include "cli/args.h"
#include "cli/prompt.h"
#include "cmd/command.h"
#include "flag.h"
#include "handlers/image.h"
#include "log.h"
#include "utils/file.h"

#include <sodium.h>
#include <stdlib.h>

static int exec(int argc, char *argv[]) {
    const char *target = shift_args(&argc, &argv);
    if (!target)
        return EXEC_USAGE_ERROR;

    char *output_file = NULL;

    // THANKS TSODING
    argc++;
    argv--;

    flag_str_var(&output_file, "o", NULL, "Output file");

    if (!flag_parse(argc, argv)) {
        flag_print_error(stderr);
        return EXEC_USAGE_ERROR;
    }

    if (!output_file)
        return EXEC_USAGE_ERROR;

    enum FileType type = get_file_type(target);
    if (type == TYPE_NOT_FOUND) {
        ERROR("Target file was not found");
        return EXEC_GENERIC_ERROR;
    }

    char passphrase[PASSPHRASE_MAX];
    if (read_passphrase("Passphrase (leave empty if the data is not encrypted): ", passphrase, sizeof(passphrase)) < 0) {
        ERROR("Failed to read the passphrase");
        return EXEC_GENERIC_ERROR;
    }

    unsigned char *data = NULL;
    size_t data_len = 0;

    DEBUG("Target: %s, Output: %s", target, output_file);

    int decode_status = -1;
    if (is_image_file(type)) {
        struct ImageCtx ctx = {
            .source_file = target,
            .output_file = output_file,

            .image_type = type,
            .codec_type = CODEC_UNKNOWN,

            .passphrase = passphrase[0] ? passphrase : NULL,
        };

        decode_status = decode_image(&ctx, &data, &data_len);
    }

    sodium_memzero(passphrase, sizeof(passphrase));
    if (decode_status < 0) {
        ERROR("Failed to decode data out of the targeted file");
        return EXEC_GENERIC_ERROR;
    }

    DEBUG("Writing %zu bytes into %s", data_len, output_file);
    int write_status = set_file_raw_data(output_file, data, data_len);

    sodium_memzero(data, data_len);
    free(data);

    if (write_status < 0) {
        ERROR("Failed to write the decoded data into %s", output_file);
        return EXEC_GENERIC_ERROR;
    }

    INFO("Decoded successfully -> %s", output_file);

    return EXEC_OK;
}

static const struct Command decode_cmd = {.name = "decode", .description = "Decodes the data hidden inside of a target file.\n", .usage = "Usage: decode <target_file> -o <output_file>\n", .exec = exec};

COMMAND(decode_cmd);
