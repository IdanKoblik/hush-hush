#pragma once

#include <string>
#include <vector>

#include <core/analysis/inspect.h>
#include <core/fs/file.h>

namespace hh {

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
