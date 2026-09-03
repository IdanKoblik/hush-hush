#pragma once

#include <sodium/crypto_secretbox.h>
#include <stddef.h>

#define NONCE_LEN crypto_secretbox_NONCEBYTES
#define KEY_LEN crypto_secretbox_KEYBYTES

struct Payload {
    unsigned char *body;
    size_t body_len;
};

struct Payload *encrypt_data(const unsigned char *data, size_t data_len, const unsigned char nonce[NONCE_LEN], const unsigned char key[KEY_LEN]);
int payload_free(struct Payload *payload);
