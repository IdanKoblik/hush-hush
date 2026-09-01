add_library(hh_testing INTERFACE)
add_library(hushhush::testing ALIAS hh_testing)

target_include_directories(hh_testing INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_link_libraries(hh_testing INTERFACE hushhush::core)
