#include "greatest.h"

#include <stdlib.h>
#include <string.h>

#include "core/analysis/search.h"

TEST parses_hex_written_any_of_the_usual_ways(void) {
    unsigned char *pattern = NULL;
    size_t len = 0;

    ASSERT_EQ(0, search_parse_hex("deadbeef", &pattern, &len));
    ASSERT_EQ((size_t)4, len);
    ASSERT_EQ(0xDE, pattern[0]);
    ASSERT_EQ(0xEF, pattern[3]);
    free(pattern);

    ASSERT_EQ(0, search_parse_hex("DE AD BE EF", &pattern, &len));
    ASSERT_EQ((size_t)4, len);
    ASSERT_EQ(0xAD, pattern[1]);
    free(pattern);

    ASSERT_EQ(0, search_parse_hex("de:ad-be_ef", &pattern, &len));
    ASSERT_EQ((size_t)4, len);
    free(pattern);

    PASS();
}

TEST rejects_hex_that_is_not_hex(void) {
    unsigned char *pattern = NULL;
    size_t len = 0;

    /* An odd digit count is half a byte. */
    ASSERT_EQ(-1, search_parse_hex("abc", &pattern, &len));
    ASSERT_EQ(-1, search_parse_hex("zz", &pattern, &len));
    ASSERT_EQ(-1, search_parse_hex("0xff", &pattern, &len));
    ASSERT_EQ(-1, search_parse_hex("", &pattern, &len));
    ASSERT_EQ(-1, search_parse_hex("   ", &pattern, &len));
    PASS();
}

TEST finds_every_match_in_order(void) {
    const unsigned char haystack[] = "the cat sat on the cat mat";
    struct SearchHits hits;

    ASSERT_EQ(0, search_bytes(haystack, sizeof(haystack) - 1, (const unsigned char *)"cat", 3, SEARCH_CASE_SENSITIVE, SEARCH_NO_LIMIT, &hits));

    ASSERT_EQ((size_t)2, hits.count);
    ASSERT_EQ((size_t)4, hits.offsets[0]);
    ASSERT_EQ((size_t)19, hits.offsets[1]);

    search_hits_free(&hits);
    PASS();
}

TEST folds_case_only_when_asked(void) {
    const unsigned char haystack[] = "Secret secret SECRET";
    struct SearchHits hits;

    ASSERT_EQ(0, search_bytes(haystack, sizeof(haystack) - 1, (const unsigned char *)"secret", 6, SEARCH_CASE_SENSITIVE, SEARCH_NO_LIMIT, &hits));
    ASSERT_EQ((size_t)1, hits.count);
    search_hits_free(&hits);

    ASSERT_EQ(0, search_bytes(haystack, sizeof(haystack) - 1, (const unsigned char *)"secret", 6, SEARCH_CASE_INSENSITIVE, SEARCH_NO_LIMIT, &hits));
    ASSERT_EQ((size_t)3, hits.count);
    search_hits_free(&hits);

    PASS();
}

TEST stops_at_the_limit(void) {
    unsigned char haystack[64];
    memset(haystack, 0xAA, sizeof(haystack));

    struct SearchHits hits;
    const unsigned char needle = 0xAA;

    ASSERT_EQ(0, search_bytes(haystack, sizeof(haystack), &needle, 1, SEARCH_CASE_SENSITIVE, 10, &hits));
    ASSERT_EQ((size_t)10, hits.count);
    search_hits_free(&hits);

    ASSERT_EQ(0, search_bytes(haystack, sizeof(haystack), &needle, 1, SEARCH_CASE_SENSITIVE, SEARCH_NO_LIMIT, &hits));
    ASSERT_EQ((size_t)64, hits.count);
    search_hits_free(&hits);

    PASS();
}

TEST reports_overlapping_matches(void) {
    const unsigned char haystack[] = "aaaa";
    struct SearchHits hits;

    ASSERT_EQ(0, search_bytes(haystack, 4, (const unsigned char *)"aa", 2, SEARCH_CASE_SENSITIVE, SEARCH_NO_LIMIT, &hits));
    ASSERT_EQ((size_t)3, hits.count);

    search_hits_free(&hits);
    PASS();
}

TEST an_impossible_needle_matches_nothing(void) {
    const unsigned char haystack[] = "short";
    struct SearchHits hits;

    ASSERT_EQ(0, search_bytes(haystack, 5, (const unsigned char *)"much longer than that", 21, SEARCH_CASE_SENSITIVE, SEARCH_NO_LIMIT, &hits));
    ASSERT_EQ((size_t)0, hits.count);
    search_hits_free(&hits);

    ASSERT_EQ(0, search_bytes(haystack, 5, (const unsigned char *)"", 0, SEARCH_CASE_SENSITIVE, SEARCH_NO_LIMIT, &hits));
    ASSERT_EQ((size_t)0, hits.count);
    search_hits_free(&hits);

    ASSERT_EQ(-1, search_bytes(NULL, 0, (const unsigned char *)"a", 1, SEARCH_CASE_SENSITIVE, SEARCH_NO_LIMIT, &hits));
    PASS();
}

SUITE(search_suite) {
    RUN_TEST(parses_hex_written_any_of_the_usual_ways);
    RUN_TEST(rejects_hex_that_is_not_hex);
    RUN_TEST(finds_every_match_in_order);
    RUN_TEST(folds_case_only_when_asked);
    RUN_TEST(stops_at_the_limit);
    RUN_TEST(reports_overlapping_matches);
    RUN_TEST(an_impossible_needle_matches_nothing);
}
