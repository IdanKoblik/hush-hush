#include "Navbar.hpp"
#include "raylib.h"
#include "render.hpp"
#include "theme.hpp"
#include <imgui.h>
#include <memory>
#include <rlImGui.h>
#include <vector>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE "Veil"

namespace {

    std::vector<std::unique_ptr<ui::Renderable>> renderables;

    void init_renderables() {
        renderables.emplace_back(std::make_unique<ui::Navbar>());
    }

} // namespace

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    rlImGuiBeginInitImGui();

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    theme::apply();

#if defined(__linux__)
    const char *fonts[] = {
        "/usr/share/fonts/TTF/JetBrainsMonoNerdFont-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/usr/share/fonts/liberation/LiberationMono-Regular.ttf",
        "/usr/share/fonts/noto/NotoSansMono-Regular.ttf",
    };

    for (const char *path : fonts) {
        if (FileExists(path)) {
            io.Fonts->AddFontFromFileTTF(path, 20.0f);
            break;
        }
    }
#endif

    ImGui::GetStyle().FontSizeBase = 20.0f;

    rlImGuiEndInitImGui();

    init_renderables();
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(Color{26, 28, 33, 255});

        rlImGuiBegin();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 9.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(18.0f, 6.0f));

        for (auto& r : renderables)
             r->render();

        ImGui::PopStyleVar(2);

        rlImGuiEnd();
        EndDrawing();
    }

    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
