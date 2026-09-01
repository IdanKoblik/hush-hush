#include "greatest.h"
#include <sodium.h>

/* Add a SUITE_EXTERN and a RUN_SUITE line per suite. */

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    if (sodium_init() < 0) {
        fprintf(stderr, "Failed to initialise libsodium\n");
        return 1;
    }

    GREATEST_MAIN_BEGIN();
    GREATEST_MAIN_END();
}
