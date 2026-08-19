#include "greatest.h"
#include <sodium.h>

SUITE_EXTERN(usage_suite);
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    if (sodium_init() < 0) {
        fprintf(stderr, "Failed to initialise libsodium\n");
        return 1;
    }

    GREATEST_MAIN_BEGIN();
    RUN_SUITE(usage_suite);
    GREATEST_MAIN_END();
}
