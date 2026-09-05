/*
 * The browser face of the core. Every entry point here is the body of one of
 * the CLI's subcommands with the argument parsing and the terminal prompt
 * taken out: the same container, the same codecs, the same statistics.
 *
 * Files are named, not passed as buffers, because that is the shape core
 * already has. Emscripten's MEMFS makes that free in the browser -- the page
 * writes an upload into /work with FS.writeFile, calls in here, and reads the
 * result back out. Nothing touches a real disk.
 *
 * Diagnostics keep going to stdout and stderr through core's own INFO/ERROR
 * macros; the page installs print and printErr hooks and shows them in its log
 * pane, so what a reader sees is what the CLI would have printed.
 */

#include <core/analysis/inspect.h>
#include <core/analysis/stats.h>
#include <core/codecs/codec.h>
#include <core/codecs/container.h>
#include <core/fs/file.h>
#include <core/handlers/image.h>
#include <core/log.h>

#include <emscripten/emscripten.h>
#include <sodium.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Json {
    char *text;
    size_t len;
    size_t cap;
    int failed;
};

static void json_raw(struct Json *json, const char *bytes, size_t len) {
    if (json->failed)
        return;

    if (json->len + len + 1 > json->cap) {
        size_t cap = json->cap ? json->cap : 256;
        while (cap < json->len + len + 1)
            cap *= 2;

        char *grown = realloc(json->text, cap);
        if (!grown) {
            json->failed = 1;
            return;
        }

        json->text = grown;
        json->cap = cap;
    }

    memcpy(json->text + json->len, bytes, len);
    json->len += len;
    json->text[json->len] = '\0';
}

static void json_text(struct Json *json, const char *literal) {
    json_raw(json, literal, strlen(literal));
}

static void json_fmt(struct Json *json, const char *fmt, ...) {
    char buffer[512];

    va_list args;
    va_start(args, fmt);
    const int written = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (written < 0 || (size_t)written >= sizeof(buffer)) {
        json->failed = 1;
        return;
    }

    json_raw(json, buffer, (size_t)written);
}

static void json_string(struct Json *json, const char *value) {
    json_text(json, "\"");

    for (const unsigned char *at = (const unsigned char *)(value ? value : ""); *at; at++) {
        switch (*at) {
        case '"':
            json_text(json, "\\\"");
            break;
        case '\\':
            json_text(json, "\\\\");
            break;
        case '\n':
            json_text(json, "\\n");
            break;
        case '\r':
            json_text(json, "\\r");
            break;
        case '\t':
            json_text(json, "\\t");
            break;
        default:
            if (*at < 0x20)
                json_fmt(json, "\\u%04x", *at);
            else
                json_raw(json, (const char *)at, 1);
        }
    }

    json_text(json, "\"");
}

static char *json_finish(struct Json *json) {
    if (!json->failed)
        return json->text;

    free(json->text);
    return NULL;
}

static size_t carrier_capacity(const struct StatSuite *suite) {
    if (suite->type == TYPE_JPEG_IMAGE)
        return suite->coefficients / 8;

    const size_t colors = pixel_color_channels(suite->channels);

    return (size_t)suite->width * (size_t)suite->height * colors / 8;
}

EMSCRIPTEN_KEEPALIVE int hh_init(void) {
    return sodium_init() < 0 ? -1 : 0;
}

EMSCRIPTEN_KEEPALIVE void hh_set_verbose(int on) {
    verbose = on;
}

/* Frees anything this file handed back as a pointer. */
EMSCRIPTEN_KEEPALIVE void hh_free(void *pointer) {
    free(pointer);
}

EMSCRIPTEN_KEEPALIVE char *hh_analyse(const char *target) {
    struct StatSuite suite;
    if (stats_analyse(target, &suite) != 0)
        return NULL;

    struct Json json = {0};

    json_text(&json, "{");
    json_text(&json, "\"type\":");
    json_string(&json, file_type_name(suite.type));
    json_fmt(&json, ",\"width\":%d,\"height\":%d,\"channels\":%d", suite.width, suite.height, suite.channels);
    json_fmt(&json, ",\"coefficients\":%zu", suite.coefficients);
    json_fmt(&json, ",\"capacity\":%zu", carrier_capacity(&suite));

    json_fmt(&json, ",\"overhead\":{\"plain\":%zu,\"encrypted\":%zu}", (size_t)(HUSH_PREAMBLE_PLAIN_LEN + HUSH_END_MARKER_LEN), (size_t)(HUSH_PREAMBLE_ENC_LEN + HUSH_HEADER_SEALED_LEN + crypto_secretbox_MACBYTES));

    json_text(&json, ",\"stats\":[");

    for (size_t i = 0; i < STAT_METHOD_COUNT; i++) {
        const enum StatMethod method = (enum StatMethod)i;
        const struct StatResult *result = &suite.results[i];

        if (i)
            json_text(&json, ",");

        json_text(&json, "{\"name\":");
        json_string(&json, stat_method_name(method));
        json_text(&json, ",\"unit\":");
        json_string(&json, stat_method_unit(method));
        json_text(&json, ",\"summary\":");
        json_string(&json, stat_method_summary(method));
        json_fmt(&json, ",\"applicable\":%s", result->applicable ? "true" : "false");

        if (result->applicable)
            json_fmt(&json, ",\"percent\":%.4f,\"reference\":%.4f,\"population\":%zu", result->percent, result->reference, result->population);

        json_text(&json, "}");
    }

    json_text(&json, "]}");

    return json_finish(&json);
}

EMSCRIPTEN_KEEPALIVE int hh_encode(const char *carrier, const char *data_file, const char *output, const char *codec_name, const char *passphrase) {
    if (!carrier || !data_file || !output)
        return -1;

    enum CodecType codec = str_to_codec(codec_name ? codec_name : "lsbm");
    if (codec == CODEC_UNKNOWN) {
        ERROR("Unknown codec: %s", codec_name ? codec_name : "(none)");
        return -1;
    }

    const enum FileType type = get_file_type(carrier);
    if (!is_image_file(type)) {
        ERROR("%s is not a PNG or a JPEG", carrier);
        return -1;
    }

    if (type == TYPE_PNG_IMAGE && codec != CODEC_LSB_REPLACEMENT && codec != CODEC_LSB_MATCHING) {
        ERROR("Invalid codec for a png image");
        return -1;
    }

    if (type == TYPE_JPEG_IMAGE)
        codec = CODEC_DCT;

    unsigned char *data = NULL;
    size_t data_len = 0;

    if (read_file_raw_data(data_file, &data, &data_len) < 0) {
        ERROR("Failed to read the data file");
        return -1;
    }

    if (data_len == 0) {
        ERROR("Cannot encode data, the data is empty");
        free(data);
        return -1;
    }

    INFO("Read %zu bytes from %s", data_len, data_file);

    struct ImageCtx ctx = {
        .source_file = carrier,
        .output_file = output,

        .image_type = type,
        .codec_type = codec,

        .passphrase = (passphrase && passphrase[0]) ? passphrase : NULL,
    };

    const int status = encode_image(&ctx, data, data_len);

    sodium_memzero(data, data_len);
    free(data);

    if (status < 0) {
        ERROR("Failed to encode data into the targeted file");
        return -1;
    }

    return 0;
}

EMSCRIPTEN_KEEPALIVE int hh_decode(const char *carrier, const char *output, const char *passphrase) {
    if (!carrier || !output)
        return -1;

    const enum FileType type = get_file_type(carrier);
    if (!is_image_file(type)) {
        ERROR("%s is not a PNG or a JPEG", carrier);
        return -1;
    }

    struct ImageCtx ctx = {
        .source_file = carrier,
        .output_file = output,

        .image_type = type,
        .codec_type = CODEC_UNKNOWN,

        .passphrase = (passphrase && passphrase[0]) ? passphrase : NULL,
    };

    unsigned char *data = NULL;
    size_t data_len = 0;

    if (decode_image(&ctx, &data, &data_len) < 0) {
        ERROR("Failed to decode data out of the targeted file");
        return -1;
    }

    const int written = write_to_file_raw_data(output, data, data_len);

    sodium_memzero(data, data_len);
    free(data);

    if (written < 0) {
        ERROR("Failed to write the decoded data into %s", output);
        return -1;
    }

    INFO("Decoded successfully -> %s", output);

    return (int)data_len;
}
