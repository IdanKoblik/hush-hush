#include "crypto/prng.h"

#include <string.h>

static void prng_refill(struct Prng *prng) {
    memset(prng->buf, 0, sizeof(prng->buf));
    crypto_stream_chacha20_ietf_xor_ic(prng->buf, prng->buf, sizeof(prng->buf), prng->nonce, prng->block, prng->key);
    prng->block += sizeof(prng->buf) / 64;
    prng->pos = 0;
}

void prng_init(struct Prng *prng, const unsigned char *key, const unsigned char *nonce) {
    memcpy(prng->key, key, sizeof(prng->key));
    memcpy(prng->nonce, nonce, sizeof(prng->nonce));
    prng->block = 0;
    prng_refill(prng);
}

uint64_t prng_next(struct Prng *prng) {
    if (prng->pos + sizeof(uint64_t) > sizeof(prng->buf))
        prng_refill(prng);

    uint64_t value;
    memcpy(&value, prng->buf + prng->pos, sizeof(value));
    prng->pos += sizeof(value);

    return value;
}

/* Unbiased value in [0, upper), same rejection scheme libsodium uses. */
uint64_t prng_uniform(struct Prng *prng, uint64_t upper) {
    if (upper < 2)
        return 0;

    uint64_t min = ((uint64_t)0 - upper) % upper;
    uint64_t value;

    do {
        value = prng_next(prng);
    } while (value < min);

    return value % upper;
}

void prng_clear(struct Prng *prng) {
    sodium_memzero(prng, sizeof(*prng));
}
