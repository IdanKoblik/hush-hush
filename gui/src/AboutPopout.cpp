#include "AboutPopout.hpp"
#include "imgui.h"

namespace ui {

AboutPopout::AboutPopout() {
}

void AboutPopout::open() {
    this->show_about = true;
}

void AboutPopout::render() {
    if (this->show_about) {
        ImGui::OpenPopup("About veil");
        this->show_about = false;
    }

    if (ImGui::BeginPopupModal("About veil", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("veil");
        ImGui::TextDisabled("A steganography toolkit.");
        ImGui::Separator();
        ImGui::TextWrapped("https://github.com/IdanKoblik/veil");
        ImGui::Separator();

        if (ImGui::Button("Close", ImVec2(120, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

} // ui
