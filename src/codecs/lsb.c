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

/*
 * stbi_load with 0 desired channels hands back 1 (grey), 2 (grey + alpha),
 * 3 (rgb) or 4 (rgba). The even counts are the ones carrying an alpha byte,
 * always last in the pixel.
 */
static size_t color_channels(const struct ImageCtx *img) {
    size_t channels = (size_t)img->channels;

    return (channels == 2 || channels == 4) ? channels - 1 : channels;
}

/*
 * Alpha carries no visible information, so touching it is both wasteful and
 * suspicious. Slots only ever address colour bytes, which means the alpha byte
 * of every pixel is stepped over: on grey + alpha that is every second byte, on
 * rgba every fourth.
 */
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

static unsigned char data_bit(const unsigned char *data, size_t index) {
    return (data[index / 8] >> (index % 8)) & 1;
}

static void embed_sequential(const struct ImageCtx *img, unsigned char *pixels, const unsigned char *data,
                             size_t data_len, size_t start_slot) {
    size_t data_bits = data_len * 8;

    for (size_t i = 0; i < data_bits; i++)
        embed_bit(img, pixels, start_slot + i, data_bit(data, i));
}

/*
 * Spreads the payload over [start_slot, capacity_slots] in a keyed pseudo random
 * order. Slots already taken are linearly probed over, which keeps the walk
 * cheap and, more importantly, reproducible by the decoder.
 */
static int embed_scattered(const struct ImageCtx *img, unsigned char *pixels, const unsigned char *data,
                           size_t data_len, size_t start_slot, size_t capacity_slots, const unsigned char *key,
                           const unsigned char *nonce) {
    size_t range = capacity_slots - start_slot;
    size_t data_bits = data_len * 8;

    unsigned char *taken = calloc((range + 7) / 8, 1);
    if (!taken) {
        ERROR("Failed to allocate the slot map");
        return -1;
    }

    struct Prng prng;
    prng_init(&prng, key, nonce);

    for (size_t i = 0; i < data_bits; i++) {
        size_t slot = (size_t)prng_uniform(&prng, range);
        while (bitmap_test(taken, slot))
            slot = (slot + 1) % range;

        bitmap_set(taken, slot);
        embed_bit(img, pixels, start_slot + slot, data_bit(data, i));
    }

    prng_clear(&prng);
    free(taken);

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

static int decode(void *ctx) {
    (void)ctx;

    ERROR("TODO IMPL");
    return -1;
}

const Codec LsbCodec = {.encode = encode, .decode = decode};
