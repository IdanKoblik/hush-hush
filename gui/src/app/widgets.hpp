#pragma once

#include "app/theme.hpp"
#include <imgui.h>

namespace ui {

inline void section_label(const char *text) {
    ImGui::PushStyleColor(ImGuiCol_Text, theme::text_dim);
    ImGui::TextUnformatted(text);
    ImGui::PopStyleColor();
};

inline void centered_label(const char *text) {
    const float x = (ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x) * 0.5f;
    if (x < ImGui::GetCursorPosX())
        return;

    ImGui::SetCursorPosX(x);
    section_label(text);
};

} // ui
