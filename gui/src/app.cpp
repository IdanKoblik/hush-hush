#include "app.hpp"
#include "theme.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include <core/analysis/search.h>

#include <GLFW/glfw3.h>
#include <imgui.h>

namespace hh {

namespace {

constexpr float splitter_width = 8.0f;
constexpr float min_pane_width = 180.0f;

constexpr float min_scale = 0.6f;
constexpr float max_scale = 3.0f;
constexpr float scale_step = 0.1f;

// Enough to page through by hand; a one byte query would otherwise find millions.
constexpr size_t max_hits = 100000;

float clamp(float value, float low, float high) {
    if (high < low)
        return low;

    return value < low ? low : (value > high ? high : value);
}

void vertical_splitter(const char *id, float height, float *width, float low, float high, bool invert) {
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton(id, ImVec2(splitter_width, height));

    if (ImGui::IsItemActive()) {
        const float delta = ImGui::GetIO().MouseDelta.x;
        *width = clamp(*width + (invert ? -delta : delta), low, high);
    }

    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    const ImVec4 color = ImGui::IsItemActive() ? theme::accent : ImGui::IsItemHovered() ? theme::accent_hover : theme::border;

    const float x = origin.x + splitter_width * 0.5f;
    ImGui::GetWindowDrawList()->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + height), ImGui::GetColorU32(color));
}

void legend_entry(ByteClass klass, const char *label) {
    const ImVec4 color = theme::byte_color(klass);

    ImGui::ColorButton(label, color, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(14.0f, 14.0f));
    ImGui::SameLine();
    ImGui::TextUnformatted(label);
}

void shortcut_row(const char *keys, const char *what) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(keys);
    ImGui::TableNextColumn();
    ImGui::PushStyleColor(ImGuiCol_Text, theme::text_dim);
    ImGui::TextUnformatted(what);
    ImGui::PopStyleColor();
}

} // namespace

App::App(GLFWwindow *window) : window_(window) {
}

App::~App() {
    release_texture();
}

void App::release_texture(void) {
    if (texture_ != 0) {
        GLuint id = texture_;
        glDeleteTextures(1, &id);
        texture_ = 0;
    }
}

void App::upload_texture(void) {
    release_texture();

    if (document_.preview.empty())
        return;

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, document_.preview_width, document_.preview_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, document_.preview.data());

    texture_ = id;
}

void App::open(const std::string &path) {
    Document document;
    const std::string failure = open_document(path, document);

    if (!failure.empty()) {
        error_ = failure;
        return;
    }

    document_ = std::move(document);
    error_.clear();
    hex_selection_ = Selection{};
    lsb_selection_ = Selection{};
    dct_selection_ = Selection{};

    search_.hits.clear();
    search_.current = -1;
    update_status();

    upload_texture();
}

void App::close_document(void) {
    document_ = Document{};
    hex_selection_ = Selection{};
    lsb_selection_ = Selection{};
    dct_selection_ = Selection{};
    search_.hits.clear();
    search_.current = -1;
    search_.open = false;
    release_texture();
}

void App::copy_selection(bool as_ascii) {
    if (!hex_selection_.active || hex_selection_.offset >= document_.bytes.size())
        return;

    const unsigned char byte = document_.bytes[hex_selection_.offset];
    char text[8];

    if (as_ascii)
        snprintf(text, sizeof(text), "%c", (byte >= 32 && byte <= 126) ? static_cast<char>(byte) : '.');
    else
        snprintf(text, sizeof(text), "%02X", byte);

    ImGui::SetClipboardText(text);
}

void App::zoom_to(float scale) {
    const float previous = ui_scale_;
    ui_scale_ = clamp(scale, min_scale, max_scale);

    const float ratio = ui_scale_ / previous;
    left_width_ *= ratio;
    right_width_ *= ratio;

    ImGui::GetStyle().FontScaleMain = ui_scale_;
}

void App::run_search(void) {
    search_.hits.clear();
    search_.current = -1;
    search_.length = 0;
    search_.status.clear();

    if (!document_.loaded() || search_.query[0] == '\0')
        return;

    const std::vector<unsigned char> &haystack = search_.target == SearchTarget::File ? document_.bytes : search_.target == SearchTarget::Lsb ? document_.lsb : document_.dct;

    unsigned char *parsed = nullptr;
    const unsigned char *needle = nullptr;
    size_t needle_len = 0;

    if (search_.mode == SearchMode::Hex) {
        if (search_parse_hex(search_.query, &parsed, &needle_len) != 0) {
            search_.status = "not hex";
            return;
        }

        needle = parsed;
    } else {
        needle = reinterpret_cast<const unsigned char *>(search_.query);
        needle_len = strlen(search_.query);
    }

    const int fold = (search_.mode == SearchMode::Text && !search_.case_sensitive) ? SEARCH_CASE_INSENSITIVE : SEARCH_CASE_SENSITIVE;

    SearchHits hits;
    if (search_bytes(haystack.data(), haystack.size(), needle, needle_len, fold, max_hits, &hits) == 0) {
        search_.hits.assign(hits.offsets, hits.offsets + hits.count);
        search_.length = needle_len;
        search_hits_free(&hits);
    }

    free(parsed);

    if (!search_.hits.empty()) {
        search_.current = 0;
        step_match(0);
    }

    update_status();
}

void App::step_match(int delta) {
    if (search_.hits.empty())
        return;

    const long count = static_cast<long>(search_.hits.size());
    search_.current = ((search_.current + delta) % count + count) % count;

    const size_t offset = search_.hits[static_cast<size_t>(search_.current)];
    Selection &selection = search_.target == SearchTarget::File ? hex_selection_ : search_.target == SearchTarget::Lsb ? lsb_selection_ : dct_selection_;

    selection.active = true;
    selection.offset = offset;
    selection.scroll_to = true;

    update_status();
}

void App::update_status(void) {
    if (search_.status == "not hex")
        return;

    if (search_.query[0] == '\0') {
        search_.status.clear();
        return;
    }

    char text[64];

    if (search_.hits.empty())
        snprintf(text, sizeof(text), "no matches");
    else if (search_.hits.size() >= max_hits)
        snprintf(text, sizeof(text), "%ld of %zu+", search_.current + 1, search_.hits.size());
    else
        snprintf(text, sizeof(text), "%ld of %zu", search_.current + 1, search_.hits.size());

    search_.status = text;
}

Highlight App::highlight_for(SearchTarget target) const {
    Highlight highlight;

    if (!search_.open || search_.target != target || search_.hits.empty())
        return highlight;

    highlight.offsets = search_.hits.data();
    highlight.count = search_.hits.size();
    highlight.length = search_.length;

    if (search_.current >= 0) {
        highlight.has_current = true;
        highlight.current = search_.hits[static_cast<size_t>(search_.current)];
    }

    return highlight;
}

void App::shortcuts(void) {
    // A popup owns Escape while it is up.
    if (ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
        return;

    ImGuiIO &io = ImGui::GetIO();

    if (io.KeyCtrl && io.MouseWheel != 0.0f) {
        zoom_to(ui_scale_ + (io.MouseWheel > 0.0f ? scale_step : -scale_step));
        io.MouseWheel = 0.0f;
    }

    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Equal) || ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_KeypadAdd))
        zoom_to(ui_scale_ + scale_step);

    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Minus) || ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_KeypadSubtract))
        zoom_to(ui_scale_ - scale_step);

    if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_0))
        zoom_to(1.0f);

    if (ImGui::IsKeyPressed(ImGuiKey_F11)) {
        if (glfwGetWindowAttrib(window_, GLFW_MAXIMIZED))
            glfwRestoreWindow(window_);
        else
            glfwMaximizeWindow(window_);
    }

    // Ctrl+C belongs to the text field while one is being typed into.
    const bool typing = io.WantTextInput;

    if (!typing) {
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_O))
            dialog_.open(std::filesystem::current_path().string());

        if (document_.loaded() && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_W))
            close_document();

        if (document_.loaded() && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_G))
            show_goto_ = true;

        if (document_.loaded() && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_F)) {
            search_.open = true;
            search_.focus = true;
        }

        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_M))
            glfwIconifyWindow(window_);

        if (hex_selection_.active && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_C))
            copy_selection(false);
    }

    if (search_.open) {
        if (ImGui::IsKeyPressed(ImGuiKey_F3))
            step_match(io.KeyShift ? -1 : 1);

        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            search_.open = false;
    }
}

void App::draw_menu_bar(void) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 9.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(18.0f, 6.0f));

    if (!ImGui::BeginMainMenuBar()) {
        ImGui::PopStyleVar(2);
        return;
    }

    menu_height_ = ImGui::GetWindowHeight();

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Open...", "Ctrl+O"))
            dialog_.open(std::filesystem::current_path().string());

        if (ImGui::MenuItem("Close", "Ctrl+W", false, document_.loaded()))
            close_document();

        ImGui::Separator();

        if (ImGui::MenuItem("Minimise", "Ctrl+M"))
            glfwIconifyWindow(window_);

        if (ImGui::MenuItem("Maximise", "F11"))
            glfwMaximizeWindow(window_);

        ImGui::Separator();

        if (ImGui::MenuItem("Exit", "Alt+F4"))
            glfwSetWindowShouldClose(window_, GLFW_TRUE);

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
        const bool has_selection = hex_selection_.active;

        if (ImGui::MenuItem("Copy byte as hex", "Ctrl+C", false, has_selection))
            copy_selection(false);

        if (ImGui::MenuItem("Copy byte as text", nullptr, false, has_selection))
            copy_selection(true);

        ImGui::Separator();

        if (ImGui::MenuItem("Find...", "Ctrl+F", false, document_.loaded())) {
            search_.open = true;
            search_.focus = true;
        }

        if (ImGui::MenuItem("Find next", "F3", false, !search_.hits.empty()))
            step_match(1);

        if (ImGui::MenuItem("Find previous", "Shift+F3", false, !search_.hits.empty()))
            step_match(-1);

        ImGui::Separator();

        if (ImGui::MenuItem("Go to offset...", "Ctrl+G", false, document_.loaded()))
            show_goto_ = true;

        ImGui::Separator();

        if (ImGui::MenuItem("Zoom in", "Ctrl++"))
            zoom_to(ui_scale_ + scale_step);

        if (ImGui::MenuItem("Zoom out", "Ctrl+-"))
            zoom_to(ui_scale_ - scale_step);

        if (ImGui::MenuItem("Reset zoom", "Ctrl+0", false, ui_scale_ != 1.0f))
            zoom_to(1.0f);

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
        if (ImGui::MenuItem("Keyboard shortcuts"))
            show_keys_ = true;

        if (ImGui::MenuItem("Byte colours"))
            show_legend_ = true;

        ImGui::Separator();

        if (ImGui::MenuItem("About Hush Hush"))
            show_about_ = true;

        ImGui::EndMenu();
    }

    const char *label = document_.loaded() ? document_.name.c_str() : "Hush Hush";
    const float centred = (ImGui::GetWindowWidth() - ImGui::CalcTextSize(label).x) * 0.5f;

    if (centred > ImGui::GetCursorPosX())
        ImGui::SetCursorPosX(centred);

    ImGui::TextDisabled("%s", label);

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 min = ImGui::GetWindowPos();
    const ImVec2 max = ImVec2(min.x + ImGui::GetWindowWidth(), min.y + ImGui::GetWindowHeight());
    draw_list->AddLine(ImVec2(min.x, max.y - 1.0f), ImVec2(max.x, max.y - 1.0f), ImGui::GetColorU32(theme::border));

    ImGui::EndMainMenuBar();
    ImGui::PopStyleVar(2);
}

void App::draw_find_bar(void) {
    if (!search_.open)
        return;

    const ImGuiStyle &style = ImGui::GetStyle();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::bar_bg);
    ImGui::BeginChild("##find", ImVec2(0.0f, ImGui::GetFrameHeight() + style.WindowPadding.y * 2.0f), ImGuiChildFlags_Borders);

    bool changed = false;

    if (search_.focus) {
        ImGui::SetKeyboardFocusHere();
        search_.focus = false;
    }

    ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0f);
    if (ImGui::InputTextWithHint("##query", search_.mode == SearchMode::Hex ? "de ad be ef" : "Find", search_.query, sizeof(search_.query), ImGuiInputTextFlags_EnterReturnsTrue))
        step_match(ImGui::GetIO().KeyShift ? -1 : 1);

    if (ImGui::IsItemEdited())
        changed = true;

    ImGui::SameLine();
    if (ImGui::RadioButton("Text", search_.mode == SearchMode::Text)) {
        search_.mode = SearchMode::Text;
        changed = true;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("Hex", search_.mode == SearchMode::Hex)) {
        search_.mode = SearchMode::Hex;
        changed = true;
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(search_.mode == SearchMode::Hex);
    if (ImGui::Checkbox("Match case", &search_.case_sensitive))
        changed = true;
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::TextDisabled("in");

    ImGui::SameLine();
    if (ImGui::RadioButton("File", search_.target == SearchTarget::File)) {
        search_.target = SearchTarget::File;
        changed = true;
    }

    ImGui::SameLine();
    if (ImGui::RadioButton("LSB", search_.target == SearchTarget::Lsb)) {
        search_.target = SearchTarget::Lsb;
        changed = true;
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(document_.dct.empty());
    if (ImGui::RadioButton("DCT", search_.target == SearchTarget::Dct)) {
        search_.target = SearchTarget::Dct;
        changed = true;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(search_.hits.empty());
    if (ImGui::ArrowButton("##previous", ImGuiDir_Up))
        step_match(-1);

    ImGui::SameLine();
    if (ImGui::ArrowButton("##next", ImGuiDir_Down))
        step_match(1);
    ImGui::EndDisabled();

    if (!search_.status.empty()) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, search_.status == "not hex" ? ImVec4(0.90f, 0.45f, 0.40f, 1.0f) : theme::text_dim);
        ImGui::TextUnformatted(search_.status.c_str());
        ImGui::PopStyleColor();
    }

    const float close = ImGui::CalcTextSize("Close").x + style.FramePadding.x * 2.0f;
    ImGui::SameLine(ImGui::GetWindowWidth() - close - style.WindowPadding.x);
    if (ImGui::Button("Close"))
        search_.open = false;

    if (changed)
        run_search();

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void App::draw_welcome(void) {
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float line = ImGui::GetTextLineHeightWithSpacing();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + available.y * 0.5f - line * 3.0f);

    const char *title = "Hush Hush";
    ImGui::SetCursorPosX((available.x - ImGui::CalcTextSize(title).x) * 0.5f);
    ImGui::TextUnformatted(title);

    const char *hint = "Open a PNG or JPEG to inspect its bytes and its least significant bits.";
    ImGui::SetCursorPosX((available.x - ImGui::CalcTextSize(hint).x) * 0.5f);
    ImGui::TextDisabled("%s", hint);

    ImGui::Spacing();

    const float button = 160.0f * ui_scale_;
    ImGui::SetCursorPosX((available.x - button) * 0.5f);
    if (ImGui::Button("Open image...", ImVec2(button, 0.0f)))
        dialog_.open(std::filesystem::current_path().string());

    const char *drop = "or drop a file onto the window";
    ImGui::SetCursorPosX((available.x - ImGui::CalcTextSize(drop).x) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, theme::text_faint);
    ImGui::TextUnformatted(drop);
    ImGui::PopStyleColor();

    if (!error_.empty()) {
        ImGui::Spacing();
        ImGui::SetCursorPosX((available.x - ImGui::CalcTextSize(error_.c_str()).x) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.45f, 0.40f, 1.0f));
        ImGui::TextUnformatted(error_.c_str());
        ImGui::PopStyleColor();
    }
}

void App::draw_status_bar(void) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::bar_bg);
    ImGui::BeginChild("##status", ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing()));

    ImGui::PushStyleColor(ImGuiCol_Text, theme::text_dim);

    if (document_.loaded())
        ImGui::Text("%s  |  %s  |  %d x %d x %d  |  %s on disk", document_.path.c_str(), type_name(document_.type), document_.width, document_.height, document_.channels, human_size(document_.bytes.size()).c_str());
    else
        ImGui::TextUnformatted(error_.empty() ? "No file" : error_.c_str());

    char text[160];
    text[0] = '\0';

    if (hex_selection_.active && hex_selection_.offset < document_.bytes.size()) {
        const unsigned char byte = document_.bytes[hex_selection_.offset];
        snprintf(text, sizeof(text), "offset 0x%08zX  |  0x%02X  |  %u  |  %c", hex_selection_.offset, byte, byte, (byte >= 32 && byte <= 126) ? static_cast<char>(byte) : '.');
    }

    if (ui_scale_ != 1.0f) {
        char zoom[32];
        snprintf(zoom, sizeof(zoom), "%s%.0f%%", text[0] != '\0' ? "  |  " : "", static_cast<double>(ui_scale_) * 100.0);
        strncat(text, zoom, sizeof(text) - strlen(text) - 1);
    }

    if (text[0] != '\0') {
        const float width = ImGui::CalcTextSize(text).x;
        ImGui::SameLine(ImGui::GetWindowWidth() - width - ImGui::GetStyle().WindowPadding.x);
        ImGui::TextUnformatted(text);
    }

    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void App::draw_streams(void) {
    if (!ImGui::BeginTabBar("##streams"))
        return;

    char summary[192];

    if (ImGui::BeginTabItem("LSB")) {
        snprintf(summary, sizeof(summary), "%zu bytes from the low bit of every colour sample, alpha skipped.", document_.lsb.size());
        draw_stream_pane("##lsbrows", summary, document_.lsb, lsb_selection_, highlight_for(SearchTarget::Lsb));
        ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem("DCT")) {
        if (document_.dct.empty()) {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, theme::text_faint);
            ImGui::TextWrapped("DCT coefficients live in JPEG files. This carrier has none.");
            ImGui::PopStyleColor();
        } else {
            snprintf(summary, sizeof(summary), "%zu bytes from the low bit of %zu usable DCT coefficients.", document_.dct.size(), document_.coefficients);
            draw_stream_pane("##dctrows", summary, document_.dct, dct_selection_, highlight_for(SearchTarget::Dct));
        }

        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
}

void App::draw_workspace(void) {
    const ImGuiViewport *viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::Begin("##workspace", nullptr, flags);
    ImGui::PopStyleVar();

    if (!document_.loaded()) {
        draw_welcome();
        ImGui::End();
        return;
    }

    draw_find_bar();

    const float minimum = min_pane_width * ui_scale_;
    const float status = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
    const float body = ImGui::GetContentRegionAvail().y - status;
    const float width = ImGui::GetContentRegionAvail().x;

    const float room = width - 2.0f * splitter_width - minimum;
    left_width_ = clamp(left_width_, minimum, room - right_width_);
    right_width_ = clamp(right_width_, minimum, room - left_width_);

    const float centre = width - left_width_ - right_width_ - 2.0f * splitter_width;

    ImGui::BeginChild("##image", ImVec2(left_width_, body), ImGuiChildFlags_Borders);
    draw_image_pane(document_, static_cast<ImTextureID>(texture_));
    ImGui::EndChild();

    ImGui::SameLine(0.0f, 0.0f);
    vertical_splitter("##split_left", body, &left_width_, minimum, room - right_width_, false);
    ImGui::SameLine(0.0f, 0.0f);

    ImGui::BeginChild("##hexpane", ImVec2(centre > 0.0f ? centre : minimum, body), ImGuiChildFlags_Borders);
    draw_hex_pane(document_, hex_selection_, highlight_for(SearchTarget::File));
    ImGui::EndChild();

    ImGui::SameLine(0.0f, 0.0f);
    vertical_splitter("##split_right", body, &right_width_, minimum, room - left_width_, true);
    ImGui::SameLine(0.0f, 0.0f);

    ImGui::BeginChild("##streampane", ImVec2(right_width_, body), ImGuiChildFlags_Borders);
    draw_streams();
    ImGui::EndChild();

    draw_status_bar();

    ImGui::End();
}

void App::draw_modals(void) {
    if (show_about_) {
        ImGui::OpenPopup("About Hush Hush");
        show_about_ = false;
    }

    if (show_legend_) {
        ImGui::OpenPopup("Byte colours");
        show_legend_ = false;
    }

    if (show_keys_) {
        ImGui::OpenPopup("Keyboard shortcuts");
        show_keys_ = false;
    }

    if (show_goto_) {
        ImGui::OpenPopup("Go to offset");
        show_goto_ = false;
    }

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const ImVec2 centre = viewport->GetCenter();

    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("About Hush Hush", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Hush Hush");
        ImGui::TextDisabled("A steganography toolkit.");
        ImGui::Separator();
        ImGui::TextWrapped("https://github.com/IdanKoblik/hush-hush");
        ImGui::Separator();

        if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Byte colours", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        legend_entry(BYTE_ZERO, "0x00");
        legend_entry(BYTE_FILLED, "0xFF");
        legend_entry(BYTE_PRINTABLE, "Printable ASCII");
        legend_entry(BYTE_WHITESPACE, "Whitespace");
        legend_entry(BYTE_CONTROL, "Control");
        legend_entry(BYTE_OTHER, "Everything else");
        ImGui::Separator();

        if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Keyboard shortcuts", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (ImGui::BeginTable("##keys", 2, ImGuiTableFlags_SizingStretchProp)) {
            shortcut_row("Ctrl+O", "Open an image");
            shortcut_row("Ctrl+W", "Close the image");
            shortcut_row("Ctrl+F", "Find in the file or the LSB stream");
            shortcut_row("F3 / Shift+F3", "Next / previous match");
            shortcut_row("Ctrl+G", "Go to offset");
            shortcut_row("Ctrl+C", "Copy the selected byte as hex");
            shortcut_row("Ctrl++ / Ctrl+-", "Zoom in / out");
            shortcut_row("Ctrl+scroll", "Zoom in / out");
            shortcut_row("Ctrl+0", "Reset zoom");
            shortcut_row("Ctrl+M", "Minimise the window");
            shortcut_row("F11", "Maximise or restore the window");
            shortcut_row("Esc", "Close the find bar");
            ImGui::EndTable();
        }

        ImGui::Separator();

        if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(centre, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Go to offset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextDisabled("Hexadecimal, into the file");

        ImGui::SetNextItemWidth(200.0f);
        const bool entered = ImGui::InputText("##offset", goto_input_, sizeof(goto_input_), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue);

        if (entered || ImGui::Button("Go", ImVec2(120.0f, 0.0f))) {
            size_t offset = 0;
            if (sscanf(goto_input_, "%zx", &offset) == 1 && offset < document_.bytes.size()) {
                hex_selection_.active = true;
                hex_selection_.offset = offset;
                hex_selection_.scroll_to = true;
            }

            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }
}

void App::frame(void) {
    shortcuts();

    draw_menu_bar();
    draw_workspace();
    draw_modals();

    std::string chosen;
    if (dialog_.draw(chosen))
        open(chosen);
}

} // namespace hh
