#pragma once

#include <stddef.h>

#define PASSPHRASE_MAX 256

int read_passphrase(const char *prompt, char *out, size_t size);

