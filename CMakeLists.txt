cmake_minimum_required(VERSION 3.21)

project(HushHush
    VERSION 1.0.0
    DESCRIPTION "Steganography toolkit"
    HOMEPAGE_URL "https://github.com/IdanKoblik/hush-hush"
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

option(HH_BUILD_CLI          "Build the hh command line app"     ON)
option(HH_BUILD_TESTS        "Build the test suite"              ${PROJECT_IS_TOP_LEVEL})
option(HH_WARNINGS_AS_ERRORS "Turn compiler warnings into errors" OFF)
option(HH_ENABLE_COVERAGE    "Instrument the build for gcov/lcov" OFF)

if(PROJECT_IS_TOP_LEVEL)
    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
            Debug Release RelWithDebInfo MinSizeRel)
    endif()

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    # .clangd looks for the compilation database at the source root.
    file(CREATE_LINK
        "${PROJECT_BINARY_DIR}/compile_commands.json"
        "${PROJECT_SOURCE_DIR}/compile_commands.json"
        SYMBOLIC RESULT hh_compdb_link
    )
    if(NOT hh_compdb_link EQUAL 0)
        message(STATUS "Could not link compile_commands.json into the source tree")
    endif()
endif()

add_library(hh_options INTERFACE)
add_library(hushhush::options ALIAS hh_options)

target_compile_definitions(hh_options INTERFACE _POSIX_C_SOURCE=200809L)

target_compile_options(hh_options INTERFACE
    $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-Wall;-Wextra>
    $<$<AND:$<BOOL:${HH_WARNINGS_AS_ERRORS}>,$<C_COMPILER_ID:GNU,Clang,AppleClang>>:-Werror>
    $<$<AND:$<BOOL:${HH_WARNINGS_AS_ERRORS}>,$<C_COMPILER_ID:MSVC>>:/WX>
)

if(HH_ENABLE_COVERAGE)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "HH_ENABLE_COVERAGE needs GCC or Clang")
    endif()
    target_compile_options(hh_options INTERFACE --coverage -O0 -g)
    target_link_options(hh_options INTERFACE --coverage)
endif()

# The shared test harness first: core/tests and cli/tests both link it, so the
# target has to exist before those directories are added.
if(HH_BUILD_TESTS)
    include(CTest)
    add_subdirectory(testing)
endif()

add_subdirectory(core)

if(HH_BUILD_CLI)
    add_subdirectory(cli)
endif()
