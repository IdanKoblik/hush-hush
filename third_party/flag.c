/*
 * flag.h is header-only: exactly one translation unit has to pull in its
 * implementation. Keeping it here rather than in cli/src/main.c means the CLI
 * test runner, which brings its own main(), gets the definitions too.
 */
#define FLAG_IMPLEMENTATION

#include "flag.h"
