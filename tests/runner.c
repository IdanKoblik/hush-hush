#include "greatest.h"
#include <sodium.h>

SUITE_EXTERN(usage_suite);
SUITE_EXTERN(command_suite);
SUITE_EXTERN(file_suite);
SUITE_EXTERN(image_suite);
SUITE_EXTERN(lsb_suite);
SUITE_EXTERN(dct_suite);
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    if (sodium_init() < 0) {
        fprintf(stderr, "Failed to initialise libsodium\n");
        return 1;
    }

    GREATEST_MAIN_BEGIN();
    RUN_SUITE(usage_suite);
    RUN_SUITE(command_suite);
    RUN_SUITE(file_suite);
    RUN_SUITE(image_suite);
    RUN_SUITE(lsb_suite);
    RUN_SUITE(dct_suite);
    GREATEST_MAIN_END();
}
