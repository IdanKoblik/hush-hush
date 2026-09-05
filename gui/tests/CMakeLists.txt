file(GLOB_RECURSE HH_GUI_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_executable(hh_gui_tests ${HH_GUI_TEST_SOURCES})
target_link_libraries(hh_gui_tests PRIVATE hushhush::gui hushhush::testing)

# No window is opened here, so the suite runs on a headless machine. Only the
# parts of the GUI that hold no ImGui state belong in it.
add_test(NAME gui COMMAND hh_gui_tests)
