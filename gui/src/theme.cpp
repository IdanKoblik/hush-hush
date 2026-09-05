#include "theme.hpp"

namespace hh::theme {

namespace {

constexpr ImVec4 byte_zero = ImVec4(0.36f, 0.38f, 0.43f, 1.00f);
constexpr ImVec4 byte_filled = ImVec4(0.85f, 0.47f, 0.36f, 1.00f);
constexpr ImVec4 byte_whitespace = ImVec4(0.38f, 0.58f, 0.56f, 1.00f);
constexpr ImVec4 byte_printable = ImVec4(0.55f, 0.76f, 0.96f, 1.00f);
constexpr ImVec4 byte_control = ImVec4(0.72f, 0.53f, 0.82f, 1.00f);
constexpr ImVec4 byte_other = ImVec4(0.74f, 0.76f, 0.80f, 1.00f);

} // namespace

ImVec4 byte_color(ByteClass klass) {
    switch (klass) {
    case BYTE_ZERO:
        return byte_zero;
    case BYTE_FILLED:
        return byte_filled;
    case BYTE_WHITESPACE:
        return byte_whitespace;
    case BYTE_PRINTABLE:
        return byte_printable;
    case BYTE_CONTROL:
        return byte_control;
    default:
        return byte_other;
    }
}

void apply(void) {
    ImGuiStyle &style = ImGui::GetStyle();

    style.WindowRounding = 0.0f;
    style.ChildRounding = 3.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;

    style.WindowBorderSize = 0.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 5.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 5.0f);
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;

    ImVec4 *colors = style.Colors;

    colors[ImGuiCol_Text] = text;
    colors[ImGuiCol_TextDisabled] = text_dim;
    colors[ImGuiCol_WindowBg] = window_bg;
    colors[ImGuiCol_ChildBg] = pane_bg;
    colors[ImGuiCol_PopupBg] = popup_bg;
    colors[ImGuiCol_Border] = border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.22f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.26f, 0.31f, 1.00f);

    colors[ImGuiCol_TitleBg] = bar_bg;
    colors[ImGuiCol_TitleBgActive] = bar_bg;
    colors[ImGuiCol_TitleBgCollapsed] = bar_bg;
    colors[ImGuiCol_MenuBarBg] = bar_bg;

    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.09f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.26f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.31f, 0.34f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = accent;

    colors[ImGuiCol_CheckMark] = accent_hover;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = accent_hover;

    colors[ImGuiCol_Button] = ImVec4(0.18f, 0.20f, 0.24f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.23f, 0.26f, 0.31f, 1.00f);
    colors[ImGuiCol_ButtonActive] = accent;

    colors[ImGuiCol_Header] = accent_soft;
    colors[ImGuiCol_HeaderHovered] = accent_hover;
    colors[ImGuiCol_HeaderActive] = accent;

    colors[ImGuiCol_Separator] = border;
    colors[ImGuiCol_SeparatorHovered] = accent;
    colors[ImGuiCol_SeparatorActive] = accent_hover;

    colors[ImGuiCol_ResizeGrip] = ImVec4(0.20f, 0.22f, 0.26f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = accent;
    colors[ImGuiCol_ResizeGripActive] = accent_hover;

    colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.13f, 0.16f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.23f, 0.28f, 1.00f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.17f, 0.19f, 0.23f, 1.00f);
    colors[ImGuiCol_TabSelectedOverline] = accent;
    colors[ImGuiCol_TabDimmed] = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);

    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = border;
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.16f, 0.17f, 0.20f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);

    colors[ImGuiCol_TextSelectedBg] = accent_soft;
    colors[ImGuiCol_NavCursor] = accent;
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.02f, 0.02f, 0.03f, 0.60f);
}

} // namespace hh::theme
