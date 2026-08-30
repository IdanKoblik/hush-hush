#ifndef CARRIER_H_
#define CARRIER_H_

#include <stddef.h>

typedef struct Carrier {
    void *ctx;
    size_t slots;

    unsigned char (*read)(const struct Carrier *carrier, size_t slot);
    void (*write)(const struct Carrier *carrier, size_t slot, unsigned char bit);
} Carrier;

#endif // CARRIER_H_
