#include "greatest.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

#include <stb_image_write.h>

#include "src/document.hpp"

static std::string write_test_png(int width, int height, int channels) {
    char path[] = "/tmp/test_gui_XXXXXX";
    const int fd = mkstemp(path);
    if (fd < 0)
        return {};

    close(fd);

    const size_t len = static_cast<size_t>(width) * height * channels;
    auto *pixels = static_cast<unsigned char *>(calloc(len, 1));
    if (!pixels) {
        unlink(path);
        return {};
    }

    for (size_t i = 0; i < len; i++)
        pixels[i] = static_cast<unsigned char>(i % 256);

    const int ok = stbi_write_png(path, width, height, channels, pixels, width * channels);
    free(pixels);

    if (!ok) {
        unlink(path);
        return {};
    }

    return path;
}

TEST opens_a_png_into_a_document(void) {
    const std::string path = write_test_png(16, 8, 3);
    ASSERT(!path.empty());

    hh::Document document;
    const std::string error = hh::open_document(path, document);

    ASSERT_STR_EQ("", error.c_str());
    ASSERT_EQ(16, document.width);
    ASSERT_EQ(8, document.height);
    ASSERT_EQ(3, document.channels);
    ASSERT_EQ(TYPE_PNG_IMAGE, document.type);
    ASSERT(document.loaded());

    ASSERT(document.bytes.size() > 0);
    ASSERT_EQ((size_t)(16 * 8 * 3 / 8), document.lsb.size());

    ASSERT_EQ((size_t)(16 * 8 * 4), document.preview.size());
    ASSERT_EQ(16, document.preview_width);
    ASSERT_EQ(8, document.preview_height);

    unlink(path.c_str());
    PASS();
}

TEST scales_a_large_preview_down(void) {
    const std::string path = write_test_png(hh::preview_max_edge * 2 + 4, 4, 3);
    ASSERT(!path.empty());

    hh::Document document;
    ASSERT_STR_EQ("", hh::open_document(path, document).c_str());

    ASSERT(document.preview_width <= hh::preview_max_edge);
    ASSERT_EQ((size_t)(document.preview_width * document.preview_height * 4), document.preview.size());

    unlink(path.c_str());
    PASS();
}

static std::string write_test_jpg(int width, int height) {
    char path[] = "/tmp/test_gui_XXXXXX";
    const int fd = mkstemp(path);
    if (fd < 0)
        return {};

    close(fd);

    const size_t len = static_cast<size_t>(width) * height * 3;
    auto *pixels = static_cast<unsigned char *>(calloc(len, 1));
    if (!pixels) {
        unlink(path);
        return {};
    }

    for (size_t i = 0; i < len; i++)
        pixels[i] = static_cast<unsigned char>(i % 256);

    const int ok = stbi_write_jpg(path, width, height, 3, pixels, 95);
    free(pixels);

    if (!ok) {
        unlink(path);
        return {};
    }

    return path;
}

TEST a_jpeg_carries_a_dct_stream(void) {
    const std::string path = write_test_jpg(64, 64);
    ASSERT(!path.empty());

    hh::Document document;
    ASSERT_STR_EQ("", hh::open_document(path, document).c_str());

    ASSERT_EQ(TYPE_JPEG_IMAGE, document.type);
    ASSERT(document.coefficients > 0);
    ASSERT_EQ(document.coefficients / 8, document.dct.size());

    // Both views are available on a JPEG: pixels carry an LSB stream too.
    ASSERT(document.lsb.size() > 0);

    unlink(path.c_str());
    PASS();
}

TEST a_png_has_no_dct_stream(void) {
    const std::string path = write_test_png(16, 8, 3);
    ASSERT(!path.empty());

    hh::Document document;
    ASSERT_STR_EQ("", hh::open_document(path, document).c_str());

    ASSERT_EQ((size_t)0, document.coefficients);
    ASSERT(document.dct.empty());

    unlink(path.c_str());
    PASS();
}

TEST refuses_what_core_cannot_decode(void) {
    char path[] = "/tmp/test_gui_XXXXXX";
    const int fd = mkstemp(path);
    ASSERT(fd >= 0);

    const char *text = "not an image";
    ASSERT(write(fd, text, strlen(text)) > 0);
    close(fd);

    hh::Document document;
    const std::string error = hh::open_document(path, document);

    ASSERT(!error.empty());
    ASSERT(!document.loaded());

    unlink(path);
    PASS();
}

TEST reports_a_missing_file(void) {
    hh::Document document;
    const std::string error = hh::open_document("/nonexistent/nope.png", document);

    ASSERT(!error.empty());
    ASSERT(!document.loaded());
    PASS();
}

TEST formats_names_and_sizes(void) {
    ASSERT_STR_EQ("cat.png", hh::file_name_of("/home/someone/cat.png").c_str());
    ASSERT_STR_EQ("cat.png", hh::file_name_of("cat.png").c_str());

    ASSERT_STR_EQ("512 B", hh::human_size(512).c_str());
    ASSERT_STR_EQ("1.0 KiB", hh::human_size(1024).c_str());
    ASSERT_STR_EQ("1.0 MiB", hh::human_size(1024 * 1024).c_str());

    ASSERT_STR_EQ("PNG", hh::type_name(TYPE_PNG_IMAGE));
    ASSERT_STR_EQ("JPEG", hh::type_name(TYPE_JPEG_IMAGE));
    PASS();
}

SUITE(document_suite) {
    RUN_TEST(opens_a_png_into_a_document);
    RUN_TEST(scales_a_large_preview_down);
    RUN_TEST(a_jpeg_carries_a_dct_stream);
    RUN_TEST(a_png_has_no_dct_stream);
    RUN_TEST(refuses_what_core_cannot_decode);
    RUN_TEST(reports_a_missing_file);
    RUN_TEST(formats_names_and_sizes);
}
