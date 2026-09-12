#include "greatest.h"
#include <sodium.h>

SUITE_EXTERN(file_suite);
SUITE_EXTERN(checksum_suite);
SUITE_EXTERN(stream_suite);
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    if (sodium_init() < 0) {
        fprintf(stderr, "Failed to initialise libsodium\n");
        return 1;
    }

    GREATEST_MAIN_BEGIN();
    RUN_SUITE(file_suite);
    RUN_SUITE(checksum_suite);
    RUN_SUITE(stream_suite);
    GREATEST_MAIN_END();
}
