#include "cmd/command.h"
#include "handlers/image.h"
#include "log.h"
#include "utils/file.h"
#include "cli/args.h"
#include "flag.h"
#include "codecs/codec.h"
#include <string.h>

static int exec(int argc, char *argv[]) {
    const char *target = shift_args(&argc, &argv);    
    const char *data_file = shift_args(&argc, &argv);
    INFO("1");
    if (!target || !data_file)
        return EXEC_USAGE_ERROR;

    char *output_file = NULL;
    char *method = NULL;
   
    // THANKS TSODING
    argc++;
    argv--;

    flag_str_var(&output_file, "o", NULL, "Output file");
    flag_str_var(&method, "c", NULL, "Encoding method");
    
    if (!flag_parse(argc, argv)) {
        flag_print_error(stderr);
        return EXEC_USAGE_ERROR;
    }
   
    if (!output_file)
        return EXEC_USAGE_ERROR;
    
    enum CodecType codec = str_to_codec(method);
    if (codec == CODEC_UNKNOWN)
        return EXEC_USAGE_ERROR;

    DEBUG("Target: %s, Data: %s, Output: %s", target, data_file, output_file);
    enum FileType type = get_file_type(target);
    if (type == TYPE_NOT_FOUND) {
        ERROR("Target file was not found");
        return EXEC_GENERIC_ERROR;
    }

    unsigned char *data = NULL;
    size_t data_len;

    DEBUG("Reading data file: %s", data_file);
    if (get_file_raw_data(data_file, &data, &data_len) < 0) {
        ERROR("Failed to get data file information");
        return EXEC_GENERIC_ERROR;
    }

    INFO("Read %zu bytes from %s", data_len, data_file);
    if (data_len <= 0) {
        ERROR("Cannot encode data, the data is empty");
        return EXEC_GENERIC_ERROR;
    }

    struct ImageCtx ctx = {
        .source_file = target,
        .output_file = output_file,

        .image_type = type,
        .codec_type = codec
    };

    int encode_status = encode_image(&ctx, data, data_len);

    free(data);
    if (encode_status < 0) {
        ERROR("Failed to encode data to the targeted file");
        return EXEC_GENERIC_ERROR;
    }

    return EXEC_OK;
}

static const struct Command encode_cmd = {.name = "encode",
                                          .description = "Encodes a data isnside of a target file.\n",
                                          .usage = "Usage: encode <target_file> <data_file> -o <output_file> -c <codec>\n",
                                          .exec = exec};

COMMAND(encode_cmd);
