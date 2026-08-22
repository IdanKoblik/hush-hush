#include "utils/file.h"
#include "handlers/image.h"

enum FileType get_file_type(const char *target) {
    enum FileType image_type = detect_image(target);
    if (image_type == TYPE_NOT_FOUND)
        return TYPE_NOT_FOUND;

    if (image_type != TYPE_UNKNOWN)
        return image_type;

    // TODO add more file types
    return TYPE_UNKNOWN; // I'm not insane, im just supporting multipale file types.
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
