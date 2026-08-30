#include "utils/file.h"
#include "handlers/image.h"

int is_image_file(enum FileType type) {
    return type == TYPE_PNG_IMAGE || type == TYPE_JPEG_IMAGE;
}

enum FileType get_file_type(const char *target) {
    enum FileType image_type = detect_image(target);
    if (image_type == TYPE_NOT_FOUND)
        return TYPE_NOT_FOUND;

    if (image_type != TYPE_UNKNOWN)
        return image_type;

    return TYPE_UNKNOWN;
}

int get_file_raw_data(const char *target, unsigned char **data, size_t *data_len) {
    FILE *file = fopen(target, "rb");
    if (!file)
        return -1;

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return -1;
    }

    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        return -1;
    }

    rewind(file);

    *data = malloc((size_t)size);
    if (!*data) {
        fclose(file);
        return -1;
    }

    *data_len = (size_t)size;
    if (fread(*data, 1, *data_len, file) != *data_len) {
        free(*data);
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

int set_file_raw_data(const char *target, const unsigned char *data, size_t data_len) {
    FILE *file = fopen(target, "wb");
    if (!file)
        return -1;

    if (data_len && fwrite(data, 1, data_len, file) != data_len) {
        fclose(file);
        return -1;
    }

    return fclose(file) == 0 ? 0 : -1;
}
