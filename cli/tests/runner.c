#include "greatest.h"

SUITE_EXTERN(usage_suite);
SUITE_EXTERN(command_suite);
SUITE_EXTERN(e2e_suite);
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(usage_suite);
    RUN_SUITE(command_suite);
    RUN_SUITE(e2e_suite);
    GREATEST_MAIN_END();
}
