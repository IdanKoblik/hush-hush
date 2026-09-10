#pragma once

#include "Document.hpp"
#include <string>
#include <veil/fs/file.h>

class FileDocument : public Document {
public:
    std::string file_name_of(const std::string &path) {
        const size_t slash = path.find_last_of('/');
        return slash == std::string::npos ? path : path.substr(slash + 1);
    }

    virtual void open(const std::string &path) = 0;
protected: 
    std::string path;
    std::string name;
    enum FileType file_type;

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
};