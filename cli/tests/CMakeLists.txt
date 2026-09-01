file(GLOB_RECURSE HH_CLI_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_executable(hh_cli_tests ${HH_CLI_TEST_SOURCES})
target_link_libraries(hh_cli_tests PRIVATE hushhush::cli hushhush::testing)

add_test(NAME cli COMMAND hh_cli_tests)
