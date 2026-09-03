file(GLOB_RECURSE HH_CLI_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
file(GLOB_RECURSE HH_CLI_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

# "/build/" keeps a stray in-tree CMake build directory from being globbed in.
list(FILTER HH_CLI_SOURCES EXCLUDE REGEX "/(tests|build)/")
list(FILTER HH_CLI_HEADERS EXCLUDE REGEX "/(tests|build)/")

# main() is kept out of the library so the test runner can bring its own.
set(HH_CLI_MAIN "")
foreach(source IN LISTS HH_CLI_SOURCES)
    get_filename_component(name "${source}" NAME)
    if(name STREQUAL "main.c")
        set(HH_CLI_MAIN "${source}")
    endif()
endforeach()

if(NOT HH_CLI_MAIN)
    message(FATAL_ERROR "No main.c under ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

list(REMOVE_ITEM HH_CLI_SOURCES "${HH_CLI_MAIN}")

if(HH_CLI_SOURCES)
    add_library(hh_cli OBJECT ${HH_CLI_SOURCES} ${HH_CLI_HEADERS})
    set(hh_cli_scope PUBLIC)
else()
    add_library(hh_cli INTERFACE)
    set(hh_cli_scope INTERFACE)
endif()
add_library(hushhush::cli ALIAS hh_cli)

target_include_directories(hh_cli ${hh_cli_scope}
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_link_libraries(hh_cli ${hh_cli_scope} hushhush::core hushhush::options)

add_executable(hh "${HH_CLI_MAIN}")
target_link_libraries(hh PRIVATE hushhush::cli)

foreach(candidate "${CMAKE_CURRENT_SOURCE_DIR}/linker.ld" "${PROJECT_SOURCE_DIR}/linker.ld")
    if(EXISTS "${candidate}")
        set(HH_LINKER_SCRIPT "${candidate}")
        break()
    endif()
endforeach()

if(HH_LINKER_SCRIPT)
    message(STATUS "Linking hh with ${HH_LINKER_SCRIPT}")
    target_link_options(hh_cli INTERFACE "LINKER:-T,${HH_LINKER_SCRIPT}")
    set_property(TARGET hh_cli APPEND
        PROPERTY INTERFACE_LINK_DEPENDS "${HH_LINKER_SCRIPT}")
endif()

install(TARGETS hh RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

if(HH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
