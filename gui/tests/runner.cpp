#include "greatest.h"

SUITE_EXTERN(document_suite);
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(document_suite);
    GREATEST_MAIN_END();
}
