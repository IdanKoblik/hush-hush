#ifndef FILE_H_
#define FILE_H_

#include <stddef.h>

enum FileType {
    TYPE_PNG_IMAGE,
    TYPE_UNKNOWN,
    TYPE_NOT_FOUND
};

enum FileType get_file_type(const char *target);
int get_file_data(const char *target, unsigned char **data, size_t *data_len);

#endif // FILE_H_
