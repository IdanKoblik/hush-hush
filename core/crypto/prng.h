#pragma once

#include <sodium.h>

#include <sodium.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PRNG_KEYBYTES crypto_stream_chacha20_ietf_KEYBYTES
#define PRNG_NONCEBYTES crypto_stream_chacha20_ietf_NONCEBYTES
#define PRNG_BUF_LEN 512

struct Prng {
    unsigned char key[PRNG_KEYBYTES];
    unsigned char nonce[PRNG_NONCEBYTES];
    unsigned char buf[PRNG_BUF_LEN];
    uint32_t block;
    size_t pos;
};

void prng_init(struct Prng *prng, const unsigned char *key, const unsigned char *nonce);
uint64_t prng_next(struct Prng *prng);
uint64_t prng_uniform(struct Prng *prng, uint64_t upper);
void prng_clear(struct Prng *prng);

#ifdef __cplusplus
}
#endif
