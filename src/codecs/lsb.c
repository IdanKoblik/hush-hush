#include "codecs/lsb.h"
#include "crypto/prng.h"
#include "log.h"
#include "stb.h"
#include "structs/image.h"

#include <sodium.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int bitmap_test(const unsigned char *bitmap, size_t index) {
    return (bitmap[index / 8] >> (index % 8)) & 1;
}

static void bitmap_set(unsigned char *bitmap, size_t index) {
    bitmap[index / 8] |= (unsigned char)(1u << (index % 8));
}

static size_t color_channels(const struct ImageCtx *img) {
    size_t channels = (size_t)img->channels;

    return (channels == 2 || channels == 4) ? channels - 1 : channels;
}

/* Skip alpha channel */
static size_t slot_to_pixel(const struct ImageCtx *img, size_t slot) {
    size_t colors = color_channels(img);
    if (colors == (size_t)img->channels)
        return slot;

    return (slot / colors) * (size_t)img->channels + (slot % colors);
}

static void lsb_replacement(unsigned char *pixel, unsigned char bit) {
    *pixel = (*pixel & LSB_FILTER) | bit;
}

static void lsb_matching(unsigned char *pixel, unsigned char bit) {
    unsigned char value = *pixel;
    if ((value & 1) == bit)
        return;

    switch (value) {
    case 0: {
        value = 1;
        break;
    }
    case 255: {
        value = 254;
        break;
    }
    default:
        value += randombytes_uniform(2) ? 1 : -1;
    }

    *pixel = value;
}

static void embed_bit(const struct ImageCtx *img, unsigned char *pixels, size_t slot, unsigned char bit) {
    unsigned char *pixel = &pixels[slot_to_pixel(img, slot)];

    switch (img->codec_type) {
    case CODEC_LSB_MATCHING: {
        lsb_matching(pixel, bit);
        break;
    }
    default:
        lsb_replacement(pixel, bit);
    }
}

static unsigned char extract_bit(const struct ImageCtx *img, const unsigned char *pixels, size_t slot) {
    return pixels[slot_to_pixel(img, slot)] & 1;
}

static unsigned char data_bit(const unsigned char *data, size_t index) {
    return (data[index / 8] >> (index % 8)) & 1;
}

/* Expects a zeroed byte, which every extraction path memsets up front. */
static void data_bit_set(unsigned char *data, size_t index, unsigned char bit) {
    data[index / 8] |= (unsigned char)(bit << (index % 8));
}

static void embed_sequential(const struct ImageCtx *img, unsigned char *pixels, const unsigned char *data,
                             size_t data_len, size_t start_slot) {
    size_t data_bits = data_len * 8;

    for (size_t i = 0; i < data_bits; i++)
        embed_bit(img, pixels, start_slot + i, data_bit(data, i));
}

static void extract_sequential(const struct ImageCtx *img, const unsigned char *pixels, unsigned char *out,
                               size_t out_len, size_t start_slot) {
    size_t out_bits = out_len * 8;

    memset(out, 0, out_len);
    for (size_t i = 0; i < out_bits; i++)
        data_bit_set(out, i, extract_bit(img, pixels, start_slot + i));
}

struct Scatter {
    struct Prng prng;
    unsigned char *taken;
    size_t range;
};

static int scatter_init(struct Scatter *walk, size_t range, const unsigned char *key, const unsigned char *nonce) {
    walk->taken = calloc((range + 7) / 8, 1);
    if (!walk->taken) {
        ERROR("Failed to allocate the slot map");
        return -1;
    }

    walk->range = range;
    prng_init(&walk->prng, key, nonce);

    return 0;
}

static size_t scatter_next(struct Scatter *walk) {
    size_t slot = (size_t)prng_uniform(&walk->prng, walk->range);

    while (bitmap_test(walk->taken, slot))
        slot = (slot + 1) % walk->range;

    bitmap_set(walk->taken, slot);

    return slot;
}

static void scatter_clear(struct Scatter *walk) {
    prng_clear(&walk->prng);
    free(walk->taken);
    walk->taken = NULL;
}

/* Spreads the payload over [start_slot, capacity_slots) in scatter order. */
static int embed_scattered(const struct ImageCtx *img, unsigned char *pixels, const unsigned char *data,
                           size_t data_len, size_t start_slot, size_t capacity_slots, const unsigned char *key,
                           const unsigned char *nonce) {
    struct Scatter walk;
    if (scatter_init(&walk, capacity_slots - start_slot, key, nonce) < 0)
        return -1;

    size_t data_bits = data_len * 8;
    for (size_t i = 0; i < data_bits; i++)
        embed_bit(img, pixels, start_slot + scatter_next(&walk), data_bit(data, i));

    scatter_clear(&walk);

    return 0;
}

static int extract_scattered(const struct ImageCtx *img, const unsigned char *pixels, unsigned char *out,
                             size_t out_len, size_t start_slot, size_t capacity_slots, const unsigned char *key,
                             const unsigned char *nonce) {
    struct Scatter walk;
    if (scatter_init(&walk, capacity_slots - start_slot, key, nonce) < 0)
        return -1;

    size_t out_bits = out_len * 8;
    memset(out, 0, out_len);
    for (size_t i = 0; i < out_bits; i++)
        data_bit_set(out, i, extract_bit(img, pixels, start_slot + scatter_next(&walk)));

    scatter_clear(&walk);

    return 0;
}

/*
 * One pwhash call produces both keys: the first half seals the header and the
 * payload, the second half drives the scatter walk.
 */
static int derive_keys(const char *passphrase, const unsigned char *salt, unsigned char *box_key,
                       unsigned char *prng_key) {
    unsigned char material[crypto_secretbox_KEYBYTES + PRNG_KEYBYTES];

    DEBUG("Deriving keys from the passphrase");
    if (crypto_pwhash(material, sizeof(material), passphrase, strlen(passphrase), salt,
                      crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE,
                      crypto_pwhash_ALG_DEFAULT) != 0) {
        ERROR("Key derivation failed, out of memory");
        return -1;
    }

    memcpy(box_key, material, crypto_secretbox_KEYBYTES);
    memcpy(prng_key, material + crypto_secretbox_KEYBYTES, PRNG_KEYBYTES);
    sodium_memzero(material, sizeof(material));

    return 0;
}

static size_t header_pack_preamble(const struct LsbHeader *header, unsigned char *out) {
    size_t off = 0;

    memcpy(out + off, header->magic, LSB_MAGIC_LEN);
    off += LSB_MAGIC_LEN;

    out[off++] = header->version;
    out[off++] = header->flags;

    if (!(header->flags & LSB_FLAG_ENCRYPTED))
        return off;

    memcpy(out + off, header->salt, sizeof(header->salt));
    off += sizeof(header->salt);

    memcpy(out + off, header->header_nonce, sizeof(header->header_nonce));
    off += sizeof(header->header_nonce);

    return off;
}

static void header_pack_body(const struct LsbHeader *header, unsigned char *out) {
    memset(out, 0, LSB_HEADER_BODY_LEN);

    out[LSB_BODY_CODEC_OFF] = header->codec;

    for (size_t i = 0; i < 4; i++)
        out[LSB_BODY_LEN_OFF + i] = (unsigned char)((header->payload_len >> (i * 8)) & 0xFF);

    memcpy(out + LSB_BODY_NONCE_OFF, header->payload_nonce, sizeof(header->payload_nonce));
}

/*
 * Reads back the cleartext part every container starts with. The salt and the
 * header nonce that follow it on an encrypted container are only pulled once the
 * flags say they are there, see header_unpack_keys.
 */
static int header_unpack_preamble(struct LsbHeader *header, const unsigned char *in) {
    memcpy(header->magic, in, LSB_MAGIC_LEN);
    header->version = in[LSB_MAGIC_LEN];
    header->flags = in[LSB_MAGIC_LEN + 1];

    if (memcmp(header->magic, LSB_MAGIC, LSB_MAGIC_LEN) != 0) {
        ERROR("No %s container was found in the image", LSB_MAGIC);
        return -1;
    }

    if (header->version != LSB_VERSION) {
        ERROR("Container version %u is not supported, this build speaks version %u", header->version, LSB_VERSION);
        return -1;
    }

    return 0;
}

static void header_unpack_keys(struct LsbHeader *header, const unsigned char *in) {
    size_t off = LSB_PREAMBLE_PLAIN_LEN;

    memcpy(header->salt, in + off, sizeof(header->salt));
    off += sizeof(header->salt);

    memcpy(header->header_nonce, in + off, sizeof(header->header_nonce));
}

static void header_unpack_body(struct LsbHeader *header, const unsigned char *in) {
    header->codec = in[LSB_BODY_CODEC_OFF];

    header->payload_len = 0;
    for (size_t i = 0; i < 4; i++)
        header->payload_len |= (uint32_t)in[LSB_BODY_LEN_OFF + i] << (i * 8);

    memcpy(header->payload_nonce, in + LSB_BODY_NONCE_OFF, sizeof(header->payload_nonce));
}

static int encode_plain(const struct ImageCtx *img, unsigned char *pixels, const unsigned char *data, size_t data_len,
                        size_t capacity_slots) {
    size_t needed = LSB_PREAMBLE_PLAIN_LEN + data_len + LSB_END_MARKER_LEN;
    if (capacity_slots / 8 < needed) {
        ERROR("Image size is not big enough to embed the targeted data into it");
        return -1;
    }

    struct LsbHeader header = {.version = LSB_VERSION, .flags = 0, .codec = (unsigned char)img->codec_type};
    memcpy(header.magic, LSB_MAGIC, LSB_MAGIC_LEN);

    unsigned char preamble[LSB_PREAMBLE_PLAIN_LEN];
    size_t preamble_len = header_pack_preamble(&header, preamble);

    DEBUG("Embedding %zu bytes sequentially, terminated by %s", data_len, LSB_END_MARKER);
    embed_sequential(img, pixels, preamble, preamble_len, 0);
    embed_sequential(img, pixels, data, data_len, preamble_len * 8);
    embed_sequential(img, pixels, (const unsigned char *)LSB_END_MARKER, LSB_END_MARKER_LEN,
                     (preamble_len + data_len) * 8);

    return 0;
}

static int encode_encrypted(const struct ImageCtx *img, unsigned char *pixels, const unsigned char *data,
                            size_t data_len, size_t capacity_slots) {
    size_t payload_len = data_len + crypto_secretbox_MACBYTES;
    size_t needed = LSB_PREAMBLE_ENC_LEN + LSB_HEADER_SEALED_LEN + payload_len;

    if (capacity_slots / 8 < needed) {
        ERROR("Image size is not big enough to embed the encrypted data into it");
        return -1;
    }

    if (payload_len > UINT32_MAX) {
        ERROR("Payload is too large to be described by the header");
        return -1;
    }

    struct LsbHeader header = {
        .version = LSB_VERSION,
        .flags = LSB_FLAG_ENCRYPTED,
        .codec = (unsigned char)img->codec_type,
        .payload_len = (uint32_t)payload_len,
    };

    memcpy(header.magic, LSB_MAGIC, LSB_MAGIC_LEN);
    randombytes_buf(header.salt, sizeof(header.salt));
    randombytes_buf(header.header_nonce, sizeof(header.header_nonce));
    randombytes_buf(header.payload_nonce, sizeof(header.payload_nonce));

    unsigned char box_key[crypto_secretbox_KEYBYTES];
    unsigned char prng_key[PRNG_KEYBYTES];
    if (derive_keys(img->passphrase, header.salt, box_key, prng_key) < 0)
        return -1;

    unsigned char body[LSB_HEADER_BODY_LEN];
    unsigned char sealed_header[LSB_HEADER_SEALED_LEN];

    header_pack_body(&header, body);
    crypto_secretbox_easy(sealed_header, body, sizeof(body), header.header_nonce, box_key);
    sodium_memzero(body, sizeof(body));

    unsigned char *payload = malloc(payload_len);
    if (!payload) {
        ERROR("Failed to allocate the payload buffer");
        sodium_memzero(box_key, sizeof(box_key));
        sodium_memzero(prng_key, sizeof(prng_key));
        return -1;
    }

    crypto_secretbox_easy(payload, data, data_len, header.payload_nonce, box_key);
    sodium_memzero(box_key, sizeof(box_key));

    unsigned char preamble[LSB_PREAMBLE_ENC_LEN];
    size_t preamble_len = header_pack_preamble(&header, preamble);
    size_t payload_start = (preamble_len + LSB_HEADER_SEALED_LEN) * 8;

    DEBUG("Embedding a %zu byte encrypted header at the start of the image", preamble_len + sizeof(sealed_header));
    embed_sequential(img, pixels, preamble, preamble_len, 0);
    embed_sequential(img, pixels, sealed_header, sizeof(sealed_header), preamble_len * 8);

    DEBUG("Scattering %zu encrypted bytes over %zu slots", payload_len, capacity_slots - payload_start);
    int status = embed_scattered(img, pixels, payload, payload_len, payload_start, capacity_slots, prng_key,
                                 header.payload_nonce);

    sodium_memzero(prng_key, sizeof(prng_key));
    sodium_memzero(payload, payload_len);
    free(payload);

    return status;
}

static int encode(void *ctx, const unsigned char *data, size_t data_len) {
    struct ImageCtx *img = ctx;

    INFO("Loading image: %s", img->source_file);
    unsigned char *pixels = stbi_load(img->source_file, &img->width, &img->height, &img->channels,
                                      0 // Any
    );

    if (!pixels) {
        ERROR("Failed to load the image (%s)", img->source_file);
        return -1;
    }

    size_t capacity_slots = (size_t)img->width * img->height * color_channels(img) * sizeof(*pixels);
    size_t capacity_bytes = capacity_slots / 8;

    int encrypted = img->passphrase && img->passphrase[0] != '\0';

    INFO("Image: %dx%d, %d channels", img->width, img->height, img->channels);
    INFO("Capacity: %zu bytes", capacity_bytes);
    INFO("Data to encode: %zu bytes", data_len);
    INFO("Encryption: %s", encrypted ? "on" : "off");

    int status = encrypted ? encode_encrypted(img, pixels, data, data_len, capacity_slots)
                           : encode_plain(img, pixels, data, data_len, capacity_slots);

    if (status < 0) {
        stbi_image_free(pixels);
        return -1;
    }

    DEBUG("Writing output image: %s", img->output_file);
    if (!stbi_write_png(img->output_file, img->width, img->height, img->channels, pixels, img->width * img->channels)) {
        stbi_image_free(pixels);
        ERROR("Failed to write into the targeted file");
        return -1;
    }

    stbi_image_free(pixels);
    INFO("Encoded successfully -> %s", img->output_file);

    return 0;
}

static int decode_plain(const struct ImageCtx *img, const unsigned char *pixels, size_t capacity_slots,
                        unsigned char **out, size_t *out_len) {
    size_t start_slot = LSB_PREAMBLE_PLAIN_LEN * 8;
    size_t max_len = capacity_slots / 8 - LSB_PREAMBLE_PLAIN_LEN;

    if (max_len <= LSB_END_MARKER_LEN) {
        ERROR("Image is too small to hold a terminated payload");
        return -1;
    }

    unsigned char *data = malloc(max_len);
    if (!data) {
        ERROR("Failed to allocate the payload buffer");
        return -1;
    }

    DEBUG("Scanning up to %zu sequential bytes for %s", max_len, LSB_END_MARKER);
    for (size_t len = 1; len <= max_len; len++) {
        extract_sequential(img, pixels, data + len - 1, 1, start_slot + (len - 1) * 8);
        if (len <= LSB_END_MARKER_LEN)
            continue;

        if (memcmp(data + len - LSB_END_MARKER_LEN, LSB_END_MARKER, LSB_END_MARKER_LEN) != 0)
            continue;

        size_t data_len = len - LSB_END_MARKER_LEN;
        unsigned char *shrunk = realloc(data, data_len);

        *out = shrunk ? shrunk : data;
        *out_len = data_len;

        return 0;
    }

    ERROR("Reached the end of the image without finding the %s marker", LSB_END_MARKER);
    free(data);

    return -1;
}

static int decode_encrypted(const struct ImageCtx *img, const unsigned char *pixels, size_t capacity_slots,
                            struct LsbHeader *header, unsigned char **out, size_t *out_len) {
    if (!img->passphrase) {
        ERROR("The container is encrypted, a passphrase is required to open it");
        return -1;
    }

    size_t header_start = LSB_PREAMBLE_ENC_LEN * 8;
    size_t payload_start = header_start + LSB_HEADER_SEALED_LEN * 8;

    if (capacity_slots <= payload_start) {
        ERROR("Image is too small to hold the encrypted header");
        return -1;
    }

    unsigned char box_key[crypto_secretbox_KEYBYTES];
    unsigned char prng_key[PRNG_KEYBYTES];
    if (derive_keys(img->passphrase, header->salt, box_key, prng_key) < 0)
        return -1;

    unsigned char sealed_header[LSB_HEADER_SEALED_LEN];
    unsigned char body[LSB_HEADER_BODY_LEN];

    size_t payload_len = 0;
    unsigned char *payload = NULL;
    unsigned char *plain = NULL;
    int status = -1;

    extract_sequential(img, pixels, sealed_header, sizeof(sealed_header), header_start);
    if (crypto_secretbox_open_easy(body, sealed_header, sizeof(sealed_header), header->header_nonce, box_key) != 0) {
        ERROR("Failed to open the header, the passphrase is wrong or the image was altered");
        goto cleanup;
    }

    header_unpack_body(header, body);
    sodium_memzero(body, sizeof(body));

    payload_len = header->payload_len;
    if (payload_len <= crypto_secretbox_MACBYTES || payload_len * 8 > capacity_slots - payload_start) {
        ERROR("The header describes a %zu byte payload, which does not fit the image", payload_len);
        goto cleanup;
    }

    payload = malloc(payload_len);
    plain = malloc(payload_len - crypto_secretbox_MACBYTES);
    if (!payload || !plain) {
        ERROR("Failed to allocate the payload buffer");
        goto cleanup;
    }

    DEBUG("Gathering %zu encrypted bytes from %zu slots", payload_len, capacity_slots - payload_start);
    if (extract_scattered(img, pixels, payload, payload_len, payload_start, capacity_slots, prng_key,
                          header->payload_nonce) < 0)
        goto cleanup;

    if (crypto_secretbox_open_easy(plain, payload, payload_len, header->payload_nonce, box_key) != 0) {
        ERROR("Failed to open the payload, the image was altered");
        goto cleanup;
    }

    *out = plain;
    *out_len = payload_len - crypto_secretbox_MACBYTES;
    plain = NULL;
    status = 0;

cleanup:
    sodium_memzero(box_key, sizeof(box_key));
    sodium_memzero(prng_key, sizeof(prng_key));

    if (payload) {
        sodium_memzero(payload, payload_len);
        free(payload);
    }

    free(plain);

    return status;
}

static int decode(void *ctx, unsigned char **data, size_t *data_len) {
    struct ImageCtx *img = ctx;

    INFO("Loading image: %s", img->source_file);
    unsigned char *pixels = stbi_load(img->source_file, &img->width, &img->height, &img->channels,
                                      0 // Any
    );

    if (!pixels) {
        ERROR("Failed to load the image (%s)", img->source_file);
        return -1;
    }

    size_t capacity_slots = (size_t)img->width * img->height * color_channels(img) * sizeof(*pixels);
    INFO("Image: %dx%d, %d channels", img->width, img->height, img->channels);

    struct LsbHeader header = {0};
    unsigned char preamble[LSB_PREAMBLE_ENC_LEN];
    int status = -1;

    if (capacity_slots / 8 < LSB_PREAMBLE_PLAIN_LEN) {
        ERROR("Image is too small to hold a %s container", LSB_MAGIC);
        goto cleanup;
    }

    extract_sequential(img, pixels, preamble, LSB_PREAMBLE_PLAIN_LEN, 0);
    if (header_unpack_preamble(&header, preamble) < 0)
        goto cleanup;

    int encrypted = (header.flags & LSB_FLAG_ENCRYPTED) != 0;
    INFO("Container: version %u, encryption %s", header.version, encrypted ? "on" : "off");

    if (!encrypted) {
        status = decode_plain(img, pixels, capacity_slots, data, data_len);
        goto cleanup;
    }

    if (capacity_slots / 8 < LSB_PREAMBLE_ENC_LEN) {
        ERROR("Image is too small to hold the encrypted preamble");
        goto cleanup;
    }

    extract_sequential(img, pixels, preamble + LSB_PREAMBLE_PLAIN_LEN, LSB_PREAMBLE_ENC_LEN - LSB_PREAMBLE_PLAIN_LEN,
                       LSB_PREAMBLE_PLAIN_LEN * 8);
    header_unpack_keys(&header, preamble);

    status = decode_encrypted(img, pixels, capacity_slots, &header, data, data_len);
    if (status == 0)
        img->codec_type = (enum CodecType)header.codec;

cleanup:
    stbi_image_free(pixels);
    if (status < 0)
        return -1;

    INFO("Decoded %zu bytes from %s", *data_len, img->source_file);

    return 0;
}

const Codec LsbCodec = {.encode = encode, .decode = decode};
