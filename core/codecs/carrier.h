#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Carrier {
    void *ctx;
    size_t slots;

    unsigned char (*read)(const struct Carrier *carrier, size_t slot);
    void (*write)(const struct Carrier *carrier, size_t slot, unsigned char bit);
} Carrier;

#ifdef __cplusplus
}
#endif
