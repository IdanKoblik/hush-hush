#pragma once

#include <string>
#include <vector>

#include <core/analysis/inspect.h>
#include <core/analysis/stats.h>
#include <core/fs/file.h>

namespace hh {

// One plot's worth of points, kept as doubles because that is what ImPlot
// reads. Every series here is a histogram, so x is a bin and y is a count.
struct Series {
    std::vector<double> x;
    std::vector<double> y;
};

// The statistical view of a carrier: the scores, and the histograms they came
// out of so the pane can show the reader the same numbers.
struct Analysis {
    StatSuite suite = {};

    Series samples;
    Series pairs;

    // The difference and coefficient histograms are split by parity, because
    // the parity is the whole story both tests are telling.
    Series differences_even;
    Series differences_odd;
    Series coefficients_even;
    Series coefficients_odd;

    bool ready = false;
};

// Differences further out than this are a long flat tail on any carrier and
// only cost the plot its detail near zero.
constexpr int difference_window = 16;

struct Document {
    std::string path;
    std::string name;

    std::vector<unsigned char> bytes;
    std::vector<unsigned char> lsb;
    std::vector<unsigned char> dct;
    size_t coefficients = 0;

    int width = 0;
    int height = 0;
    int channels = 0;
    FileType type = TYPE_UNKNOWN;

    Analysis analysis;

    std::vector<unsigned char> preview;
    int preview_width = 0;
    int preview_height = 0;

    bool loaded() const {
        return !path.empty();
    }
};

// Preview only, so anything larger is sampled down before it reaches the GPU.
constexpr int preview_max_edge = 1024;

// Returns an empty string on success, or why the file could not be opened.
std::string open_document(const std::string &path, Document &out);

std::string file_name_of(const std::string &path);
const char *type_name(FileType type);
std::string human_size(size_t bytes);

} // namespace hh
