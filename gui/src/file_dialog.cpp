#include "file_dialog.hpp"
#include "theme.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>

#include <imgui.h>

namespace fs = std::filesystem;

namespace hh {

namespace {

bool is_supported(const fs::path &path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return extension == ".png" || extension == ".jpg" || extension == ".jpeg";
}

} // namespace

void FileDialog::open(const std::string &start_directory) {
    pending_ = true;

    if (directory_.empty())
        navigate(start_directory);
}

void FileDialog::navigate(const std::string &directory) {
    std::error_code code;
    const fs::path canonical = fs::weakly_canonical(directory, code);
    const fs::path target = code ? fs::path(directory) : canonical;

    entries_.clear();
    selected_ = -1;
    error_.clear();

    fs::directory_iterator it(target, fs::directory_options::skip_permission_denied, code);
    if (code) {
        error_ = "Cannot open " + target.string();
        return;
    }

    directory_ = target.string();
    std::snprintf(input_, sizeof(input_), "%s", directory_.c_str());

    for (const fs::directory_entry &entry : it) {
        const std::string name = entry.path().filename().string();
        if (!name.empty() && name[0] == '.')
            continue;

        std::error_code kind;
        if (entry.is_directory(kind))
            entries_.push_back({name, true});
        else if (is_supported(entry.path()))
            entries_.push_back({name, false});
    }

    std::sort(entries_.begin(), entries_.end(), [](const Entry &a, const Entry &b) {
        if (a.directory != b.directory)
            return a.directory;

        return a.name < b.name;
    });
}

bool FileDialog::draw(std::string &chosen) {
    if (pending_) {
        ImGui::OpenPopup("Open image");
        pending_ = false;
    }

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->GetCenter().x, viewport->GetCenter().y), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(620.0f, 460.0f), ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal("Open image", nullptr, ImGuiWindowFlags_NoSavedSettings))
        return false;

    bool picked = false;
    std::string pick;

    if (ImGui::Button("Up") && !directory_.empty())
        navigate(fs::path(directory_).parent_path().string());

    ImGui::SameLine();

    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputText("##path", input_, sizeof(input_), ImGuiInputTextFlags_EnterReturnsTrue)) {
        std::error_code code;
        if (fs::is_directory(input_, code))
            navigate(input_);
        else if (fs::is_regular_file(input_, code)) {
            pick = input_;
            picked = true;
        } else
            error_ = std::string("No such file or directory: ") + input_;
    }

    if (!error_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.45f, 0.40f, 1.0f));
        ImGui::TextWrapped("%s", error_.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Separator();

    const float footer = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
    ImGui::BeginChild("##entries", ImVec2(0.0f, -footer), ImGuiChildFlags_Borders);

    for (int i = 0; i < static_cast<int>(entries_.size()); i++) {
        const Entry &entry = entries_[static_cast<size_t>(i)];

        ImGui::PushStyleColor(ImGuiCol_Text, entry.directory ? theme::text : theme::accent_hover);
        const std::string label = entry.directory ? entry.name + "/" : entry.name;

        if (ImGui::Selectable(label.c_str(), selected_ == i, ImGuiSelectableFlags_AllowDoubleClick)) {
            selected_ = i;

            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                const std::string path = (fs::path(directory_) / entry.name).string();

                if (entry.directory) {
                    ImGui::PopStyleColor();
                    navigate(path);
                    break;
                }

                pick = path;
                picked = true;
            }
        }

        ImGui::PopStyleColor();
    }

    ImGui::EndChild();

    const bool has_file = selected_ >= 0 && selected_ < static_cast<int>(entries_.size()) && !entries_[static_cast<size_t>(selected_)].directory;

    ImGui::BeginDisabled(!has_file);
    if (ImGui::Button("Open", ImVec2(110.0f, 0.0f)) && has_file) {
        pick = (fs::path(directory_) / entries_[static_cast<size_t>(selected_)].name).string();
        picked = true;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(110.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
        ImGui::CloseCurrentPopup();

    if (picked) {
        chosen = pick;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
    return picked;
}

} // namespace hh
