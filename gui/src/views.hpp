#pragma once

#include <cstddef>
#include <imgui.h>
#include <vector>

#include "document.hpp"

namespace hh {

struct Selection {
    bool active = false;
    size_t offset = 0;

    // Cleared by the pane once it has scrolled.
    bool scroll_to = false;
};

struct Highlight {
    const size_t *offsets = nullptr;
    size_t count = 0;
    size_t length = 0;

    bool has_current = false;
    size_t current = 0;
};

void draw_image_pane(const Document &doc, ImTextureID texture);
void draw_hex_pane(const Document &doc, Selection &selection, const Highlight &highlight);
void draw_stream_pane(const char *id, const char *summary, const std::vector<unsigned char> &bytes, Selection &selection, const Highlight &highlight);

} // namespace hh
