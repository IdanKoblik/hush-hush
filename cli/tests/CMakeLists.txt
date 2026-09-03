file(GLOB_RECURSE HH_CLI_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_executable(hh_cli_tests ${HH_CLI_TEST_SOURCES})
target_link_libraries(hh_cli_tests PRIVATE hushhush::cli hushhush::testing)

# e2e.c runs the real binary instead of linking it, so it needs the path and a
# guarantee that hh is built and up to date first.
target_compile_definitions(hh_cli_tests PRIVATE "HH_BINARY=\"$<TARGET_FILE:hh>\"")
add_dependencies(hh_cli_tests hh)

add_test(NAME cli COMMAND hh_cli_tests)
