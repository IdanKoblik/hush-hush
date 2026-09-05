#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SEARCH_NO_LIMIT 0
#define SEARCH_CASE_SENSITIVE 0
#define SEARCH_CASE_INSENSITIVE 1

struct SearchHits {
    size_t *offsets;
    size_t count;
};

int search_parse_hex(const char *text, unsigned char **pattern, size_t *pattern_len);
int search_bytes(const unsigned char *haystack, size_t haystack_len, const unsigned char *needle, size_t needle_len, int case_insensitive, size_t limit, struct SearchHits *out);

void search_hits_free(struct SearchHits *hits);

#ifdef __cplusplus
}
#endif
