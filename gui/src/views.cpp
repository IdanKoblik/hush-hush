#include "views.hpp"
#include "theme.hpp"

#include <algorithm>
#include <cstdio>

namespace hh {

namespace {

constexpr size_t hex_columns = 16;
constexpr size_t hex_group = 8;

// The bundled font is fixed width, so one glyph advance drives every column.
struct Metrics {
    float glyph;
    float line;
    float offset_w;
    float cell_w;
    float group_gap;
    float ascii_gap;
    float total_w;
};

Metrics hex_metrics(void) {
    Metrics m;

    m.glyph = ImGui::CalcTextSize("0").x;
    m.line = ImGui::GetTextLineHeight();
    m.offset_w = m.glyph * 10.0f;
    m.cell_w = m.glyph * 3.0f;
    m.group_gap = m.glyph * 1.5f;
    m.ascii_gap = m.glyph * 2.0f;
    m.total_w = m.offset_w + hex_columns * m.cell_w + m.group_gap + m.ascii_gap + hex_columns * m.glyph;

    return m;
}

float hex_column_x(const Metrics &m, size_t column) {
    return m.offset_w + column * m.cell_w + (column >= hex_group ? m.group_gap : 0.0f);
}

float ascii_column_x(const Metrics &m, size_t column) {
    return m.offset_w + hex_columns * m.cell_w + m.group_gap + m.ascii_gap + column * m.glyph;
}

// Offsets are sorted, so the enclosing match is the last one starting at or before this byte.
bool highlighted(const Highlight &highlight, size_t offset, bool &current) {
    current = false;

    if (!highlight.offsets || highlight.count == 0 || highlight.length == 0)
        return false;

    const size_t *end = highlight.offsets + highlight.count;
    const size_t *it = std::upper_bound(highlight.offsets, end, offset);

    if (it == highlight.offsets)
        return false;

    --it;

    if (offset >= *it + highlight.length)
        return false;

    current = highlight.has_current && *it == highlight.current;
    return true;
}

void section_label(const char *label) {
    ImGui::PushStyleColor(ImGuiCol_Text, theme::text_dim);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
}

// Drawn into the draw list, not widgets: the clipper needs each row exactly one line tall.
void draw_hex_row(const unsigned char *data, size_t size, size_t base, const Metrics &m, bool window_hovered, Selection &selection, const Highlight &highlight) {
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    char text[24];

    snprintf(text, sizeof(text), "%08zX", base);
    draw_list->AddText(origin, ImGui::GetColorU32(theme::text_faint), text);

    const ImU32 rule = ImGui::GetColorU32(ImVec4(theme::border.x, theme::border.y, theme::border.z, 0.5f));
    const float rule_x = origin.x + m.offset_w - m.glyph;
    draw_list->AddLine(ImVec2(rule_x, origin.y), ImVec2(rule_x, origin.y + m.line), rule);

    const float ascii_rule_x = origin.x + ascii_column_x(m, 0) - m.glyph;
    draw_list->AddLine(ImVec2(ascii_rule_x, origin.y), ImVec2(ascii_rule_x, origin.y + m.line), rule);

    for (size_t column = 0; column < hex_columns; column++) {
        const size_t offset = base + column;
        if (offset >= size)
            break;

        const unsigned char byte = data[offset];
        const ImU32 color = ImGui::GetColorU32(theme::byte_color(classify_byte(byte)));

        const float x = origin.x + hex_column_x(m, column);
        const ImVec2 cell_min(x - m.glyph * 0.35f, origin.y);
        const ImVec2 cell_max(x + m.glyph * 2.35f, origin.y + m.line);

        const bool hovered = window_hovered && ImGui::IsMouseHoveringRect(cell_min, cell_max);
        const bool picked = selection.active && selection.offset == offset;

        bool current = false;
        const bool found = highlighted(highlight, offset, current);

        if (found)
            draw_list->AddRectFilled(cell_min, cell_max, ImGui::GetColorU32(current ? theme::match_current : theme::match), 2.0f);

        if (picked)
            draw_list->AddRectFilled(cell_min, cell_max, ImGui::GetColorU32(theme::accent_soft), 2.0f);
        else if (hovered && !found)
            draw_list->AddRectFilled(cell_min, cell_max, ImGui::GetColorU32(ImGuiCol_FrameBgHovered), 2.0f);

        if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            selection.active = true;
            selection.offset = offset;
        }

        snprintf(text, sizeof(text), "%02X", byte);
        draw_list->AddText(ImVec2(x, origin.y), color, text);

        const char glyph = (byte >= 32 && byte <= 126) ? static_cast<char>(byte) : '.';
        const float ascii_x = origin.x + ascii_column_x(m, column);

        if (picked)
            draw_list->AddRectFilled(ImVec2(ascii_x, origin.y), ImVec2(ascii_x + m.glyph, origin.y + m.line), ImGui::GetColorU32(theme::accent_soft), 2.0f);

        draw_list->AddText(ImVec2(ascii_x, origin.y), color, &glyph, &glyph + 1);
    }

    ImGui::Dummy(ImVec2(m.total_w, m.line));
}

void draw_hex_header(const Metrics &m) {
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImU32 color = ImGui::GetColorU32(theme::text_dim);

    char text[8];

    draw_list->AddText(origin, color, "Offset");

    for (size_t column = 0; column < hex_columns; column++) {
        snprintf(text, sizeof(text), "%02zX", column);
        draw_list->AddText(ImVec2(origin.x + hex_column_x(m, column), origin.y), color, text);
    }

    draw_list->AddText(ImVec2(origin.x + ascii_column_x(m, 0), origin.y), color, "ASCII");

    ImGui::Dummy(ImVec2(m.total_w, m.line));
    ImGui::Separator();
}

void draw_stream_row(const unsigned char *data, size_t base, const Metrics &m, bool window_hovered, Selection &selection, const Highlight &highlight) {
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();

    const unsigned char byte = data[base];

    InspectRow row;
    inspect_row(byte, &row);

    const float binary_x = m.glyph * 10.0f;
    const float hex_x = binary_x + m.glyph * 10.0f;
    const float ascii_x = hex_x + m.glyph * 6.0f;
    const float width = ascii_x + m.glyph * 2.0f;

    const ImVec2 row_min(origin.x, origin.y);
    const ImVec2 row_max(origin.x + width, origin.y + m.line);

    const bool hovered = window_hovered && ImGui::IsMouseHoveringRect(row_min, row_max);
    const bool picked = selection.active && selection.offset == base;

    bool current = false;
    const bool found = highlighted(highlight, base, current);

    if (found)
        draw_list->AddRectFilled(row_min, row_max, ImGui::GetColorU32(current ? theme::match_current : theme::match), 2.0f);

    if (picked)
        draw_list->AddRectFilled(row_min, row_max, ImGui::GetColorU32(theme::accent_soft), 2.0f);
    else if (hovered && !found)
        draw_list->AddRectFilled(row_min, row_max, ImGui::GetColorU32(ImGuiCol_FrameBgHovered), 2.0f);

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        selection.active = true;
        selection.offset = base;
    }

    char text[24];
    snprintf(text, sizeof(text), "%08zX", base);

    const ImU32 color = ImGui::GetColorU32(theme::byte_color(classify_byte(byte)));

    draw_list->AddText(origin, ImGui::GetColorU32(theme::text_faint), text);
    draw_list->AddText(ImVec2(origin.x + binary_x, origin.y), ImGui::GetColorU32(theme::text_dim), row.binary);
    draw_list->AddText(ImVec2(origin.x + hex_x, origin.y), color, row.hex);
    draw_list->AddText(ImVec2(origin.x + ascii_x, origin.y), color, row.ascii);

    ImGui::Dummy(ImVec2(width, m.line));
}

void draw_recovered_text(const std::vector<unsigned char> &bytes) {
    constexpr size_t peek = 512;

    if (!ImGui::CollapsingHeader("Recovered text", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    std::string text;
    const size_t count = bytes.size() < peek ? bytes.size() : peek;
    text.reserve(count);

    for (size_t i = 0; i < count; i++) {
        const unsigned char byte = bytes[i];
        text.push_back((byte >= 32 && byte <= 126) ? static_cast<char>(byte) : '.');
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.09f, 0.10f, 0.12f, 1.0f));
    ImGui::BeginChild("##recovered", ImVec2(0.0f, ImGui::GetTextLineHeight() * 5.0f), ImGuiChildFlags_Borders);
    ImGui::PushStyleColor(ImGuiCol_Text, theme::text_dim);
    ImGui::TextWrapped("%s", text.c_str());
    ImGui::PopStyleColor();
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void metadata_row(const char *label, const char *value) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::PushStyleColor(ImGuiCol_Text, theme::text_dim);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(value);
}

} // namespace

void draw_image_pane(const Document &doc, ImTextureID texture) {
    section_label("IMAGE");
    ImGui::Spacing();

    if (texture != 0 && doc.preview_width > 0) {
        const float available = ImGui::GetContentRegionAvail().x;
        const float scale = available / static_cast<float>(doc.preview_width);
        const ImVec2 size(available, static_cast<float>(doc.preview_height) * scale);

        const ImVec2 min = ImGui::GetCursorScreenPos();
        ImGui::Image(texture, size);
        ImGui::GetWindowDrawList()->AddRect(min, ImVec2(min.x + size.x, min.y + size.y), ImGui::GetColorU32(theme::border));
    }

    ImGui::Spacing();
    ImGui::Spacing();
    section_label("PROPERTIES");
    ImGui::Spacing();

    if (!ImGui::BeginTable("##properties", 2, ImGuiTableFlags_SizingStretchProp))
        return;

    char value[128];

    metadata_row("Name", doc.name.c_str());
    metadata_row("Format", type_name(doc.type));

    snprintf(value, sizeof(value), "%d x %d", doc.width, doc.height);
    metadata_row("Size", value);

    snprintf(value, sizeof(value), "%d", doc.channels);
    metadata_row("Channels", value);

    metadata_row("On disk", human_size(doc.bytes.size()).c_str());
    metadata_row("LSB capacity", human_size(doc.lsb.size()).c_str());

    if (doc.coefficients > 0) {
        snprintf(value, sizeof(value), "%zu", doc.coefficients);
        metadata_row("Coefficients", value);
        metadata_row("DCT capacity", human_size(doc.dct.size()).c_str());
    }

    ImGui::EndTable();
}

void draw_hex_pane(const Document &doc, Selection &selection, const Highlight &highlight) {
    section_label("HEX");
    ImGui::Spacing();

    const Metrics m = hex_metrics();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    draw_hex_header(m);

    ImGui::BeginChild("##hex", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

    if (selection.scroll_to) {
        ImGui::SetScrollY(static_cast<float>(selection.offset / hex_columns) * m.line);
        selection.scroll_to = false;
    }

    const bool hovered = ImGui::IsWindowHovered();
    const size_t rows = (doc.bytes.size() + hex_columns - 1) / hex_columns;

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(rows), m.line);

    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
            draw_hex_row(doc.bytes.data(), doc.bytes.size(), static_cast<size_t>(row) * hex_columns, m, hovered, selection, highlight);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

void draw_stream_pane(const char *id, const char *summary, const std::vector<unsigned char> &bytes, Selection &selection, const Highlight &highlight) {
    ImGui::PushStyleColor(ImGuiCol_Text, theme::text_faint);
    ImGui::TextWrapped("%s", summary);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    draw_recovered_text(bytes);
    ImGui::Spacing();

    const Metrics m = hex_metrics();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const ImVec2 header = ImGui::GetCursorScreenPos();
    const ImU32 dim = ImGui::GetColorU32(theme::text_dim);

    draw_list->AddText(header, dim, "Offset");
    draw_list->AddText(ImVec2(header.x + m.glyph * 10.0f, header.y), dim, "Binary");
    draw_list->AddText(ImVec2(header.x + m.glyph * 20.0f, header.y), dim, "Hex");
    draw_list->AddText(ImVec2(header.x + m.glyph * 26.0f, header.y), dim, "Chr");

    ImGui::Dummy(ImVec2(m.glyph * 28.0f, m.line));
    ImGui::Separator();

    ImGui::BeginChild(id, ImVec2(0.0f, 0.0f), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

    if (selection.scroll_to) {
        ImGui::SetScrollY(static_cast<float>(selection.offset) * m.line);
        selection.scroll_to = false;
    }

    const bool hovered = ImGui::IsWindowHovered();

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(bytes.size()), m.line);

    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
            draw_stream_row(bytes.data(), static_cast<size_t>(row), m, hovered, selection, highlight);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
}

} // namespace hh
