#pragma once

#include "render.hpp"
#include <string>

namespace ui {

const std::string about_popout = "About Veil";

class AboutPopout : public Renderable {
public:
    AboutPopout();
    ~AboutPopout() override = default;

    void open();
    void render() override;
private:
    bool show_about = false;

};

} // ui
