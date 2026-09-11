#include "ImageDocument.hpp"
#include "raylib.h"
#include "theme.hpp"
#include <algorithm>
#include <imgui.h>
#include <memory>
#include <rlImGui.h>
#include <stdexcept>
#include <vector>
#include <veil/fs/file.h>
#include <veil/log.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE "Veil"

static void load_fonts(void) {
    ImGuiIO &io = ImGui::GetIO();

#if defined(__linux__)
    const char *fonts[] = {
        "/usr/share/fonts/TTF/JetBrainsMonoNerdFont-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
        "/usr/share/fonts/liberation/LiberationMono-Regular.ttf",
        "/usr/share/fonts/noto/NotoSansMono-Regular.ttf",
    };

    for (const char *path : fonts)
        if (FileExists(path) && io.Fonts->AddFontFromFileTTF(path, 20.0f))
            return;
#endif

    io.Fonts->AddFontDefault();
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    rlImGuiSetLoadFontsCallback(load_fonts);
    rlImGuiBeginInitImGui();

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    theme::apply();

    ImGui::GetStyle().FontSizeBase = 20.0f;

    rlImGuiEndInitImGui();

    const ImGuiViewport *viewport = ImGui::GetMainViewport();

    // Scoped so the document's texture is released while the GL context still exists.
    {
        ImageDocument document;
        try {
            document.open("/home/idank/Pictures/78589468.jpg"); // TODO
        } catch (const std::exception &e) {
            ERROR("%s", e.what());
        }

        while (!WindowShouldClose()) {
            BeginDrawing();
            ClearBackground(Color{26, 28, 33, 255});

            rlImGuiBegin();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 9.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(18.0f, 6.0f));

            document.draw_navbar();

            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::Begin("##workspace", nullptr,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                         ImGuiWindowFlags_NoBackground
            );

            document.render();

            ImGui::End();

            ImGui::PopStyleVar(2);

            rlImGuiEnd();
            EndDrawing();
        }
    }

    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
