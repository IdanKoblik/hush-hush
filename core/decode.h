#pragma once

#include <veil/limits.h>
#include <stddef.h>

int decode(const char *target, const char passphrase[PASSPHRASE_MAX], unsigned char **data, size_t *data_len);
