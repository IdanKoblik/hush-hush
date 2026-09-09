#pragma once

namespace ui {

class Renderable {
public:
    virtual ~Renderable() = default;
    virtual void render() = 0;
};

} // ui
