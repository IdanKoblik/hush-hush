#include "greatest.h"
#include <sodium.h>

SUITE_EXTERN(image_suite);
SUITE_EXTERN(lsb_suite);
SUITE_EXTERN(dct_suite);
SUITE_EXTERN(file_suite);
SUITE_EXTERN(inspect_suite);
SUITE_EXTERN(search_suite);
SUITE_EXTERN(dct_inspect_suite);
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    if (sodium_init() < 0) {
        fprintf(stderr, "Failed to initialise libsodium\n");
        return 1;
    }

    GREATEST_MAIN_BEGIN();
    RUN_SUITE(image_suite);
    RUN_SUITE(lsb_suite);
    RUN_SUITE(dct_suite);
    RUN_SUITE(file_suite);
    RUN_SUITE(inspect_suite);
    RUN_SUITE(search_suite);
    RUN_SUITE(dct_inspect_suite);
    GREATEST_MAIN_END();
}
