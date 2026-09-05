#include "document.hpp"

#include <core/analysis/dct.h>

#include <cstdio>
#include <cstdlib>

namespace hh {

namespace {

void fill_analysis(FileType type, const PixelBuffer &pixels, const DctCoefficients *coefficients, Analysis &out) {
    if (stats_run(type, &pixels, coefficients, &out.suite) != 0)
        return;

    size_t samples[STAT_SAMPLE_LEVELS];
    if (stats_sample_histogram(&pixels, samples) == 0) {
        for (int value = 0; value < STAT_SAMPLE_LEVELS; value++) {
            out.samples.x.push_back(value);
            out.samples.y.push_back(static_cast<double>(samples[value]));
        }

        for (int value = 0; value < STAT_SAMPLE_LEVELS; value += 2) {
            const double total = static_cast<double>(samples[value]) + static_cast<double>(samples[value + 1]);
            if (total <= 0.0)
                continue;

            out.pairs.x.push_back(value);
            out.pairs.y.push_back(100.0 * static_cast<double>(samples[value]) / total);
        }
    }

    size_t differences[STAT_DIFFERENCE_LEVELS];
    if (stats_difference_histogram(&pixels, differences) == 0) {
        for (int difference = -difference_window; difference <= difference_window; difference++) {
            Series &series = (difference % 2 == 0) ? out.differences_even : out.differences_odd;

            series.x.push_back(difference);
            series.y.push_back(static_cast<double>(differences[difference + STAT_DIFFERENCE_ZERO]));
        }
    }

    size_t values[STAT_COEFFICIENT_LEVELS];
    if (coefficients && stats_coefficient_histogram(coefficients, values) == 0) {
        for (int value = -STAT_COEFFICIENT_RANGE; value <= STAT_COEFFICIENT_RANGE; value++) {
            Series &series = (value % 2 == 0) ? out.coefficients_even : out.coefficients_odd;

            series.x.push_back(value);
            series.y.push_back(static_cast<double>(values[value + STAT_COEFFICIENT_ZERO]));
        }
    }

    out.ready = true;
}

void build_preview(const PixelBuffer &pixels, Document &doc) {
    int scale = 1;
    while (pixels.width / scale > preview_max_edge || pixels.height / scale > preview_max_edge)
        scale++;

    const int width = pixels.width / scale > 0 ? pixels.width / scale : 1;
    const int height = pixels.height / scale > 0 ? pixels.height / scale : 1;
    const int channels = pixels.channels;

    doc.preview.assign(static_cast<size_t>(width) * height * 4, 255);
    doc.preview_width = width;
    doc.preview_height = height;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const size_t src = (static_cast<size_t>(y) * scale * pixels.width + static_cast<size_t>(x) * scale) * channels;
            const size_t dst = (static_cast<size_t>(y) * width + x) * 4;

            if (src + channels > pixels.len)
                continue;

            switch (channels) {
            case 1:
                doc.preview[dst + 0] = doc.preview[dst + 1] = doc.preview[dst + 2] = pixels.samples[src];
                break;
            case 2:
                doc.preview[dst + 0] = doc.preview[dst + 1] = doc.preview[dst + 2] = pixels.samples[src];
                doc.preview[dst + 3] = pixels.samples[src + 1];
                break;
            case 4:
                doc.preview[dst + 3] = pixels.samples[src + 3];
                [[fallthrough]];
            default:
                doc.preview[dst + 0] = pixels.samples[src + 0];
                doc.preview[dst + 1] = pixels.samples[src + 1];
                doc.preview[dst + 2] = pixels.samples[src + 2];
                break;
            }
        }
    }
}

} // namespace

std::string file_name_of(const std::string &path) {
    const size_t slash = path.find_last_of('/');

    return slash == std::string::npos ? path : path.substr(slash + 1);
}

const char *type_name(FileType type) {
    return file_type_name(type);
}

std::string human_size(size_t bytes) {
    char buffer[64];

    if (bytes >= 1024u * 1024u)
        snprintf(buffer, sizeof(buffer), "%.1f MiB", static_cast<double>(bytes) / (1024.0 * 1024.0));
    else if (bytes >= 1024u)
        snprintf(buffer, sizeof(buffer), "%.1f KiB", static_cast<double>(bytes) / 1024.0);
    else
        snprintf(buffer, sizeof(buffer), "%zu B", bytes);

    return buffer;
}

std::string open_document(const std::string &path, Document &out) {
    const FileType type = get_file_type(path.c_str());

    if (type == TYPE_NOT_FOUND)
        return "No such file: " + path;

    if (!is_image_file(type))
        return "Not a PNG or JPEG image: " + file_name_of(path);

    unsigned char *raw = nullptr;
    size_t raw_len = 0;
    if (read_file_raw_data(path.c_str(), &raw, &raw_len) != 0)
        return "Could not read " + file_name_of(path);

    PixelBuffer pixels;
    if (pixels_load(path.c_str(), &pixels) != 0) {
        free(raw);
        return "Could not decode " + file_name_of(path);
    }

    LsbStream stream;
    if (inspect_lsb(&pixels, NO_LIMIT, &stream) != 0) {
        pixels_free(&pixels);
        free(raw);
        return "Could not read the low bits of " + file_name_of(path);
    }

    Document doc;
    doc.path = path;
    doc.name = file_name_of(path);
    doc.type = type;
    doc.width = pixels.width;
    doc.height = pixels.height;
    doc.channels = pixels.channels;
    doc.bytes.assign(raw, raw + raw_len);
    doc.lsb.assign(stream.bytes, stream.bytes + stream.len);

    DctCoefficients coefficients;
    const bool has_coefficients = type == TYPE_JPEG_IMAGE && dct_load(path.c_str(), &coefficients) == 0;

    if (has_coefficients) {
        DctStream dct;

        if (inspect_dct(&coefficients, NO_LIMIT, &dct) == 0) {
            doc.dct.assign(dct.bytes, dct.bytes + dct.len);
            doc.coefficients = coefficients.count;
            dct_stream_free(&dct);
        }
    }

    fill_analysis(type, pixels, has_coefficients ? &coefficients : nullptr, doc.analysis);

    if (has_coefficients)
        dct_free(&coefficients);

    build_preview(pixels, doc);

    lsb_stream_free(&stream);
    pixels_free(&pixels);
    free(raw);

    out = std::move(doc);
    return {};
}

} // namespace hh
