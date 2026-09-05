#include "../args.h"
#include "../prompt.h"
#include "command.h"
#include "core/fs/checksum.h"
#include <core/codecs/codec.h>
#include <core/fs/file.h>
#include <core/handlers/image.h>
#include <core/log.h>
#include <flag.h>
#include <sodium.h>
#include <stdlib.h>
#include <string.h>

static int exec(int argc, char *argv[]) {
    const char *target = shift_args(&argc, &argv);
    const char *data_file = shift_args(&argc, &argv);
    if (!target || !data_file)
        return EXEC_USAGE_ERROR;

    char *output_file = NULL;
    char *method = NULL;

    // THANKS TSODING
    argc++;
    argv--;

    bool verify = false;
    flag_str_var(&output_file, "o", NULL, "Output file");
    flag_str_var(&method, "c", "lsbm", "Encoding method");
    flag_bool_var(&verify, "verify", false, "Check encoding result");

    if (!flag_parse(argc, argv)) {
        flag_print_error(stderr);
        return EXEC_USAGE_ERROR;
    }

    if (!output_file)
        return EXEC_USAGE_ERROR;

    enum CodecType codec = str_to_codec(method);
    if (codec == CODEC_UNKNOWN)
        return EXEC_USAGE_ERROR;

    enum FileType type = get_file_type(target);
    if (type == TYPE_NOT_FOUND) {
        ERROR("Target file was not found");
        return EXEC_GENERIC_ERROR;
    }

    if (!is_image_file(type)) {
        ERROR("UNSUPPORTED");
        return EXEC_GENERIC_ERROR;
    }

    if (type == TYPE_PNG_IMAGE && (codec != CODEC_LSB_REPLACEMENT && codec != CODEC_LSB_MATCHING)) {
        ERROR("Invalid codec for a png image");
        return EXEC_GENERIC_ERROR;
    } else if (type == TYPE_JPEG_IMAGE) {
        codec = CODEC_DCT;
    }

    char passphrase[PASSPHRASE_MAX];
    if (read_passphrase("Passphrase (leave empty to disable encryption): ", passphrase, sizeof(passphrase)) < 0) {
        ERROR("Failed to read the passphrase");
        return EXEC_GENERIC_ERROR;
    }

    unsigned char *data = NULL;
    size_t data_len;

    DEBUG("Target: %s, Data: %s, Output: %s", target, data_file, output_file);
    DEBUG("Reading data file: %s", data_file);
    if (read_file_raw_data(data_file, &data, &data_len) < 0) {
        ERROR("Failed to get data file information");
        return EXEC_GENERIC_ERROR;
    }

    INFO("Read %zu bytes from %s", data_len, data_file);
    if (data_len <= 0) {
        ERROR("Cannot encode data, the data is empty");
        return EXEC_GENERIC_ERROR;
    }

    if (is_image_file(type)) {
        struct ImageCtx ctx = {.source_file = target,
                               .output_file = output_file,

                               .image_type = type,
                               .codec_type = codec,

                               .passphrase = passphrase[0] ? passphrase : NULL};

        if (encode_image(&ctx, data, data_len) < 0) {
            ERROR("Failed to encode data to the targeted file");
            goto fail;
        }

        if (verify) {
            struct ImageCtx check = ctx;
            check.source_file = output_file;
            check.codec_type = CODEC_UNKNOWN;

            unsigned char *output_data = NULL;
            size_t output_data_len = 0;

            if (decode_image(&check, &output_data, &output_data_len) < 0) {
                ERROR("Failed to decode the encoded target");
                goto fail;
            }

            bool verified = false;

            if (output_data_len != data_len)
                ERROR("Read %zu bytes back out of %s but encoded %zu, encoding failed", output_data_len, output_file, data_len);
            else if (calculate_checksum(data, data_len) != calculate_checksum(output_data, output_data_len))
                ERROR("What came back out of %s is not what went in, encoding failed", output_file);
            else
                verified = true;

            sodium_memzero(output_data, output_data_len);
            free(output_data);

            if (!verified)
                goto fail;

            INFO("Verified %zu bytes read back from %s", data_len, output_file);
        }
    }

    sodium_memzero(passphrase, sizeof(passphrase));
    free(data);

    return EXEC_OK;
fail:
    sodium_memzero(passphrase, sizeof(passphrase));
    free(data);
    return EXEC_GENERIC_ERROR;
}

static const struct Command encode_cmd = {.name = "encode", .description = "Encodes a data isnside of a target file.", .usage = "Usage: encode <target_file> <data_file> -o <output_file> -c <codec> [-verify]\n", .exec = exec};

COMMAND(encode_cmd);
