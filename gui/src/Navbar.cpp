#include "Navbar.hpp"
#include "AboutPopout.hpp"
#include "imgui.h"

namespace ui {

Navbar::Navbar() {
    this->about_popout = AboutPopout();
}

void Navbar::render() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Open...", "Ctrl+O");
            ImGui::MenuItem("Close", "Ctrl+W");

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Find", "Ctrl+F")) {
                this->show_finder = true;
                this->finder_focus = true;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("help")) {
            if (ImGui::MenuItem("About veil"))
                about_popout.open();

            ImGui::EndMenu();
        }

        about_popout.render();

        ImGui::EndMainMenuBar();
    }
}

} // ui
