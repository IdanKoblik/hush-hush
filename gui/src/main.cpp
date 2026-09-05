#include <GLFW/glfw3.h>
#include <core/log.h>
#include <cstdio>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>

#include "app.hpp"
#include "theme.hpp"

static void on_glfw_error(int code, const char *description) {
    fprintf(stderr, "[-] GLFW error %d: %s\n", code, description);
}

static void on_file_dropped(GLFWwindow *window, int count, const char **paths) {
    hh::App *app = static_cast<hh::App *>(glfwGetWindowUserPointer(window));

    if (app && count > 0)
        app->open(paths[0]);
}

int main(int argc, char **argv) {
    glfwSetErrorCallback(on_glfw_error);
    if (!glfwInit()) {
        ERROR("Failed to initialise GLFW");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(1280, 720, "Hush Hush", nullptr, nullptr);
    if (!window) {
        ERROR("Failed to create the window");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    hh::theme::apply();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Scoped so the texture is released while the GL context is still alive.
    {
        hh::App app(window);
        if (argc > 1)
            app.open(argv[1]);

        glfwSetWindowUserPointer(window, &app);
        glfwSetDropCallback(window, on_file_dropped);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            app.frame();

            ImGui::Render();

            int width = 0, height = 0;
            glfwGetFramebufferSize(window, &width, &height);
            glViewport(0, 0, width, height);

            glClearColor(hh::theme::window_bg.x, hh::theme::window_bg.y, hh::theme::window_bg.z, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }

        glfwSetDropCallback(window, nullptr);
        glfwSetWindowUserPointer(window, nullptr);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
