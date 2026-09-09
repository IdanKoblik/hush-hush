#pragma once

#include "AboutPopout.hpp"
#include "render.hpp"

namespace ui {

class Navbar : public Renderable {
public:
    Navbar();
    ~Navbar() override = default;

    void render() override;
private:
    bool show_finder = false;
    bool finder_focus = false;

    AboutPopout about_popout;
};

} // ui
