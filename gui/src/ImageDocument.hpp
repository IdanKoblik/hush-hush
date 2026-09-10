#pragma once

#include "FileDocument.hpp"
#include "math.hpp"
#include "theme.hpp"
#include <sstream>
#include <iostream>
#include <string>
#include <imgui.h>
#include <memory>
#include <rlImGui.h>
#include <stdexcept>
#include <vector>
#include <veil/analysis/inspect.h>
#include <veil/fs/file.h>
#include "Preview.hpp"

class ImageDocument : public FileDocument {
public:
    void render(void) override {
        const float status_h = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
        const float body = ImGui::GetContentRegionAvail().y - status_h;
        const float total = ImGui::GetContentRegionAvail().x;
        const float room = total - 16.0f - 180.0f;

        this->state.left_w = clampf(this->state.left_w, 180.0f, room - this->state.right_w);
        this->state.right_w = clampf(this->state.right_w, 180.0f, room - this->state.left_w);

        ImGui::BeginChild("##source", ImVec2(this->state.left_w, body), ImGuiChildFlags_Borders);
        this->section_label("PREVIEW");
        ImGui::Spacing();
        this->preview.draw(ImGui::GetContentRegionAvail().x);
        ImGui::Spacing();
        ImGui::Spacing();
        this->section_label("PROPERTIES");
        ImGui::Spacing();
        ImGui::TextUnformatted(this->name.c_str());
        ImGui::Text("%s, %zu bytes", file_type_name(this->file_type), this->bytes.size());
        ImGui::Text("%d x %d, %d channels", this->pixels.buffer.width, this->pixels.buffer.height,
                    this->pixels.buffer.channels);
        ImGui::EndChild();
    
        this->draw_status_bar();
    }

    void open(const std::string &target) override {
        const std::string target_name = file_name_of(target);

        this->file_type = get_file_type(target.c_str());
        if (this->file_type == TYPE_NOT_FOUND)
            throw std::runtime_error("No such file: " + target);

        if (!is_image_file(this->file_type))
            throw std::runtime_error("Not a valid supported image type: " + target_name);

        unsigned char *data = nullptr;
        size_t data_len = 0;
        if (read_file_raw_data(target.c_str(), &data, &data_len) != 0)
            throw std::runtime_error("Could not read " + target_name);

        const std::unique_ptr<unsigned char, void (*)(void *)> raw(data, std::free);

        Pixels decoded;
        if (pixels_load(target.c_str(), &decoded.buffer) != 0)
            throw std::runtime_error("Could not decode " + target_name);

        std::vector<unsigned char> raw_bytes(raw.get(), raw.get() + data_len);

        std::swap(this->pixels.buffer, decoded.buffer);
        this->bytes = std::move(raw_bytes);
        this->path = target;
        this->name = target_name;

        this->preview.load(this->pixels.buffer);
    };

    std::string summary() override {
        std::stringstream ss;
        ss << this->path << "  | " << file_type_name(this->file_type);
        return ss.str();
    };
private:
    Pixels pixels;
    std::vector<unsigned char> bytes;

    Preview preview;
};
