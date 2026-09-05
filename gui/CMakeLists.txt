file(GLOB_RECURSE HH_GUI_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE HH_GUI_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

list(FILTER HH_GUI_SOURCES EXCLUDE REGEX "/(tests|build|_deps)/")
list(FILTER HH_GUI_HEADERS EXCLUDE REGEX "/(tests|build|_deps)/")

# main() is kept out of the library so a test runner can bring its own.
set(HH_GUI_MAIN "")
foreach(source IN LISTS HH_GUI_SOURCES)
    get_filename_component(name "${source}" NAME)
    if(name STREQUAL "main.cpp")
        set(HH_GUI_MAIN "${source}")
    endif()
endforeach()

if(NOT HH_GUI_MAIN)
    message(FATAL_ERROR "No main.cpp under ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

list(REMOVE_ITEM HH_GUI_SOURCES "${HH_GUI_MAIN}")

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

# GLFW the way core resolves libsodium: the installation's own CMake package
# when it ships one, pkg-config otherwise.
find_package(glfw3 3.3 QUIET)
if(glfw3_FOUND)
    set(HH_GLFW glfw)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(HH_GLFW_PC QUIET IMPORTED_TARGET glfw3)
    endif()

    if(NOT HH_GLFW_PC_FOUND)
        message(FATAL_ERROR
            "Could not find GLFW 3. Install its development package "
            "(libglfw3-dev on Debian and Ubuntu), or point CMAKE_PREFIX_PATH "
            "at your own build of it.")
    endif()

    set(HH_GLFW PkgConfig::HH_GLFW_PC)
endif()

set(HH_IMGUI_TAG  "v1.92.9b" CACHE STRING "Dear ImGui version to build against")
set(HH_IMPLOT_TAG "v1.0"     CACHE STRING "ImPlot version to build against")

include(FetchContent)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG "${HH_IMGUI_TAG}"
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR do-not-configure
)

FetchContent_Declare(implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG "${HH_IMPLOT_TAG}"
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR do-not-configure
)

FetchContent_MakeAvailable(imgui implot)

# ImGui, its GLFW/OpenGL3 backends and ImPlot in one library. It is deliberately
# not linked against hushhush::options: this is vendored third-party code and is
# built without our warning flags, exactly like third_party/ in core.
#
# imgui_demo.cpp and implot_demo.cpp are left out.
add_library(hh_imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
    "${implot_SOURCE_DIR}/implot.cpp"
    "${implot_SOURCE_DIR}/implot_items.cpp"
)

target_include_directories(hh_imgui SYSTEM PUBLIC
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
    "${implot_SOURCE_DIR}"
)

target_link_libraries(hh_imgui PUBLIC ${HH_GLFW} OpenGL::GL)

target_compile_features(hh_imgui PUBLIC cxx_std_17)

set_target_properties(hh_imgui PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(hh_imgui PRIVATE -w)
endif()

if(HH_GUI_SOURCES)
    add_library(hh_gui OBJECT ${HH_GUI_SOURCES} ${HH_GUI_HEADERS})
    set(hh_gui_scope PUBLIC)
else()
    add_library(hh_gui INTERFACE)
    set(hh_gui_scope INTERFACE)
endif()
add_library(hushhush::gui ALIAS hh_gui)

target_include_directories(hh_gui ${hh_gui_scope}
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_link_libraries(hh_gui ${hh_gui_scope} hushhush::core hushhush::options hh_imgui)
target_compile_features(hh_gui ${hh_gui_scope} cxx_std_17)

add_executable(hh-gui "${HH_GUI_MAIN}")
target_link_libraries(hh-gui PRIVATE hushhush::gui)

install(TARGETS hh-gui RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

if(HH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
