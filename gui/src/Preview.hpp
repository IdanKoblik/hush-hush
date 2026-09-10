#pragma once

#include <imgui.h>
#include <rlImGui.h>
#include <veil/analysis/inspect.h>
#include <math.hpp>
#include <algorithm>
#include <cstdlib>

struct Pixels {
    struct PixelBuffer buffer {};

    Pixels() = default;
    Pixels(const Pixels &) = delete;
    Pixels &operator=(const Pixels &) = delete;

    ~Pixels() {
        pixels_free(&this->buffer);
    }
};

class Preview {
public:
    static constexpr int max_edge = 1024;

    Preview() = default;
    Preview(const Preview &) = delete;
    Preview &operator=(const Preview &) = delete;

    ~Preview();

    void load(const struct PixelBuffer &pixels); 
    void unload(void);
    void draw(float edge);

private:
    Texture2D texture {};

    static int format_of(int channels) {
        switch (channels) {
        case 1: return PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
        case 2: return PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA;
        case 3: return PIXELFORMAT_UNCOMPRESSED_R8G8B8;
        case 4: return PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        default: return 0;
        }
    }
};
