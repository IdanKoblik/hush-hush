#include "src/theme.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <implot.h>
#include <veil/log.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define WINDOW_TITLE "veil"

namespace {

void on_glfw_error(int code, const char *description) {
    ERROR("[-] GLFW error %d: %s\n", code, description);
}

} // namespace

int main(void) {
    glfwSetErrorCallback(on_glfw_error);
    if (!glfwInit()) {
        ERROR("Failed to init GLFW");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, nullptr, nullptr);
    if (!window) {
        ERROR("Failed to create the window");
        goto fail;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    theme::apply();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Scoped so the texture is released while the GL context is still alive.
    {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
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
fail:
    if (window)
        glfwDestroyWindow(window);

    glfwTerminate();
    return 1;
}
