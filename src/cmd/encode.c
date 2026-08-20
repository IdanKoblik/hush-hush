#include "cmd/encode.h"
#include "cmd/command.h"
#include "handlers/image.h"
#include "utils/file.h"
#include "log.h"
#include <string.h>

int exec(int argc, char *argv[]) {
    if (argc < 4)
        return EXEC_USAGE_ERROR;

    const char *target = argv[0];
    const char *data_file = argv[1];
    const char *flag = argv[2];
    if (strcmp(flag, "-o") != 0)
        return EXEC_GENERIC_ERROR;

    const char *flag_value = argv[3];

    enum FileType type = get_file_type(target);
    if (type == TYPE_NOT_FOUND) {
        ERROR("Target file was not found");
        return EXEC_GENERIC_ERROR;
    }

    unsigned char *data = NULL;
    size_t data_len;
    if (get_file_data(data_file, &data, &data_len) < 0) {
        ERROR("Failed to get data file information");
        return EXEC_GENERIC_ERROR;
    }

    int encode_status = 0;
    switch (type) {
        case TYPE_PNG_IMAGE: {
            INFO("Encoding data inside of %s", target);
            encode_status = encode_image(target, type, flag_value, data, data_len);
            break;
        }
        default: {
            ERROR("Unsupported file type");
            free(data);
            return EXEC_GENERIC_ERROR;
        }
    }

    free(data);
    if (encode_status < 0) {
        ERROR("Failed to encode data to the targeted file");
        return EXEC_GENERIC_ERROR;
    }

    return EXEC_OK;
}

static const struct Command encode_cmd = {
    .name = ENCODE_CMD_NAME,
    .description = "Encodes a data isnside of a target file.\n",
    .usage = "encode <target_file> <data_file> -o <output_file>\n",
    .exec = exec
};

COMMAND(encode_cmd);
