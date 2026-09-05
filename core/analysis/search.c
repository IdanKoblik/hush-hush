#include "search.h"

#include <ctype.h>
#include <stdlib.h>

static int hex_digit(char c) {
    if (c >= '0' && c <= '9')
        return c - '0';

    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return -1;
}

static int is_separator(char c) {
    return c == ' ' || c == '\t' || c == ',' || c == '-' || c == ':' || c == '_';
}

int search_parse_hex(const char *text, unsigned char **pattern, size_t *pattern_len) {
    if (!text || !pattern || !pattern_len)
        return -1;

    size_t digits = 0;
    for (const char *it = text; *it; it++) {
        if (is_separator(*it))
            continue;

        if (hex_digit(*it) < 0)
            return -1;

        digits++;
    }

    if (digits == 0 || digits % 2 != 0)
        return -1;

    unsigned char *bytes = calloc(digits / 2, sizeof(*bytes));
    if (!bytes)
        return -1;

    size_t index = 0;
    int high = -1;

    for (const char *it = text; *it; it++) {
        if (is_separator(*it))
            continue;

        const int value = hex_digit(*it);

        if (high < 0) {
            high = value;
            continue;
        }

        bytes[index++] = (unsigned char)((high << 4) | value);
        high = -1;
    }

    *pattern = bytes;
    *pattern_len = digits / 2;
    return 0;
}

static int hits_push(struct SearchHits *hits, size_t *capacity, size_t offset) {
    if (hits->count == *capacity) {
        const size_t next = *capacity > 0 ? *capacity * 2 : 64;
        size_t *grown = realloc(hits->offsets, next * sizeof(*grown));

        if (!grown)
            return -1;

        hits->offsets = grown;
        *capacity = next;
    }

    hits->offsets[hits->count++] = offset;
    return 0;
}

int search_bytes(const unsigned char *haystack, size_t haystack_len, const unsigned char *needle, size_t needle_len, int case_insensitive, size_t limit, struct SearchHits *out) {
    if (!out)
        return -1;

    out->offsets = NULL;
    out->count = 0;

    if (!haystack || !needle)
        return -1;

    if (needle_len == 0 || needle_len > haystack_len)
        return 0;

    size_t capacity = 0;
    const size_t last = haystack_len - needle_len;

    for (size_t i = 0; i <= last; i++) {
        size_t matched = 0;

        while (matched < needle_len) {
            unsigned char a = haystack[i + matched];
            unsigned char b = needle[matched];

            if (case_insensitive) {
                a = (unsigned char)tolower(a);
                b = (unsigned char)tolower(b);
            }

            if (a != b)
                break;

            matched++;
        }

        if (matched != needle_len)
            continue;

        if (hits_push(out, &capacity, i) != 0) {
            search_hits_free(out);
            return -1;
        }

        if (limit != SEARCH_NO_LIMIT && out->count >= limit)
            break;
    }

    return 0;
}

void search_hits_free(struct SearchHits *hits) {
    if (!hits)
        return;

    free(hits->offsets);
    hits->offsets = NULL;
    hits->count = 0;
}
