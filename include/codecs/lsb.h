#ifndef LSB_H_
#define LSB_H_

#include "codec.h"

#include <sodium.h>

#define LSB_FILTER 0XFE

/* Container magic, kept in cleartext so a reader can tell the two modes apart. */
#define LSB_MAGIC "HUSH"
#define LSB_MAGIC_LEN 4
#define LSB_VERSION 1

/* Header flags. */
#define LSB_FLAG_ENCRYPTED 0x01

/* Terminates the payload when no passphrase was supplied. */
#define LSB_END_MARKER "$$END$$"
#define LSB_END_MARKER_LEN (sizeof(LSB_END_MARKER) - 1)

/* magic | version | flags */
#define LSB_PREAMBLE_PLAIN_LEN (LSB_MAGIC_LEN + 2)
/* magic | version | flags | salt | header nonce */
#define LSB_PREAMBLE_ENC_LEN (LSB_PREAMBLE_PLAIN_LEN + crypto_pwhash_SALTBYTES + crypto_secretbox_NONCEBYTES)

/* codec | reserved[3] | payload_len | payload nonce */
#define LSB_HEADER_BODY_LEN (8 + crypto_secretbox_NONCEBYTES)
#define LSB_HEADER_SEALED_LEN (LSB_HEADER_BODY_LEN + crypto_secretbox_MACBYTES)

/*
 * Container layout, expressed in "slots". A slot is one usable pixel byte, and
 * carries exactly one bit of the payload in its LSB. On RGBA carriers the alpha
 * byte is skipped, so slot n is not always pixel byte n (see slot_to_pixel).
 *
 * Encrypted (a passphrase was supplied):
 *
 *   [ magic | version | flags | salt | nonce ]  cleartext preamble, sequential from slot 0
 *   [ secretbox(codec | len | payload nonce) ]  encrypted header, sequential
 *   [ secretbox(data) ]                         scattered over the remaining slots by the
 *                                               passphrase derived PRNG
 *
 * The preamble has to stay in the clear: without the salt and the nonce there is
 * no way to re-derive the key and open the header. Everything that describes the
 * payload (its length, where the scatter walk starts from) lives in the sealed
 * header instead.
 *
 * Plain (no passphrase):
 *
 *   [ magic | version | flags ]                 cleartext preamble, sequential from slot 0
 *   [ data ][ $$END$$ ]                         sequential, the marker ends the payload
 */

/* Offsets inside the (sealed) header body. */
#define LSB_BODY_CODEC_OFF 0
#define LSB_BODY_LEN_OFF 4
#define LSB_BODY_NONCE_OFF 8

struct LsbHeader {
    unsigned char magic[LSB_MAGIC_LEN];
    unsigned char version;
    unsigned char flags;
    unsigned char codec;

    uint32_t payload_len;

    unsigned char salt[crypto_pwhash_SALTBYTES];
    unsigned char header_nonce[crypto_secretbox_NONCEBYTES];
    unsigned char payload_nonce[crypto_secretbox_NONCEBYTES];
};

extern const Codec LsbCodec;

#endif // LSB_H_
