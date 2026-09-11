#pragma once

#include "ViewState.hpp"
#include <imgui.h>
#include "math.hpp"
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

    void draw_navbar(void) {
        if (!ImGui::BeginMainMenuBar())
            return;

        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("Open...", "Ctrl+O");
            ImGui::MenuItem("Close", "Ctrl+W");
            this->file_menu();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Find", "Ctrl+F", &this->state.find_open);
            ImGui::Separator();

            if (ImGui::MenuItem("Zoom in", "Ctrl++"))
                this->zoom_by(ViewState::zoom_step);

            if (ImGui::MenuItem("Zoom out", "Ctrl+-"))
                this->zoom_by(1.0f / ViewState::zoom_step);

            if (ImGui::MenuItem("Reset zoom", "Ctrl+0"))
                this->state.zoom = 1.0f;

            this->edit_menu();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About Veil"))
                this->show_about = true;
    
            ImGui::EndMenu();
        }

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

        this->extra_menus();
        this->navbar_center();

        ImGui::EndMainMenuBar();
    };

    inline void section_label(const char *s) {
        ImGui::PushStyleColor(ImGuiCol_Text, theme::text_dim);
        ImGui::TextUnformatted(s);
        ImGui::PopStyleColor();
    };

    void draw_status_bar(void) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::bar_bg);
        ImGui::BeginChild("##status", ImVec2(0, ImGui::GetTextLineHeightWithSpacing()));
        ImGui::PushStyleColor(ImGuiCol_Text, theme::text_dim);
        ImGui::Text(this->summary().c_str());

        // TODO right side 

        ImGui::PopStyleColor();
        ImGui::EndChild();
        ImGui::PopStyleColor();
    };

protected:
    ViewState state;

    virtual void file_menu(void) {};
    virtual void edit_menu(void) {};
    virtual void extra_menus(void) {};
    virtual void navbar_center(void) {};

    inline void navbar_label(const char *s) {
        const float x = (ImGui::GetWindowWidth() - ImGui::CalcTextSize(s).x) * 0.5f;
        if (x < ImGui::GetCursorPosX())
            return;

        ImGui::SetCursorPosX(x);
        ImGui::PushStyleColor(ImGuiCol_Text, theme::text_dim);
        ImGui::TextUnformatted(s);
        ImGui::PopStyleColor();
    };

    inline void zoom_by(float factor) {
        this->state.zoom = clampf(this->state.zoom * factor, ViewState::zoom_min, ViewState::zoom_max);
    };
private:
    bool show_about = false;
};
