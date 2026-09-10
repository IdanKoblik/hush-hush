#pragma once

#include "ViewState.hpp"
#include <imgui.h>
#include "theme.hpp"
#include <string>

class Document {
public:
    Document() {
        this->state = ViewState();
    };    

    virtual ~Document() = default;
    virtual void render(void) = 0;
    virtual std::string summary(void) = 0;

    inline void section_label(const char *s) {
        ImGui::PushStyleColor(ImGuiCol_Text, theme::text_dim);
        ImGui::TextUnformatted(s);
        ImGui::PopStyleColor();
    };

protected:
    ViewState state;
};