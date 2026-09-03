#pragma once

#include <stddef.h>

typedef struct Carrier {
    void *ctx;
    size_t slots;

    unsigned char (*read)(const struct Carrier *carrier, size_t slot);
    void (*write)(const struct Carrier *carrier, size_t slot, unsigned char bit);
} Carrier;


