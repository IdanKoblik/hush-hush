file(GLOB_RECURSE HH_CORE_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

add_executable(hh_core_tests ${HH_CORE_TEST_SOURCES})
target_link_libraries(hh_core_tests PRIVATE hushhush::core hushhush::testing)

add_test(NAME core COMMAND hh_core_tests)
