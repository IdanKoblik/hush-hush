#include "checksum.h"

unsigned char calculate_checksum(const unsigned char *data, size_t data_len) {
    unsigned char sum = 0;
    for (size_t i = 0; i < data_len; i++)
        sum += data[i];

    return sum;
}
