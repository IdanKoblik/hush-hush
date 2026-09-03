file(GLOB_RECURSE HH_CORE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
file(GLOB_RECURSE HH_CORE_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

list(FILTER HH_CORE_SOURCES EXCLUDE REGEX "/tests/")
list(FILTER HH_CORE_HEADERS EXCLUDE REGEX "/tests/")

if(NOT HH_CORE_SOURCES)
    message(FATAL_ERROR "No .c files under ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

find_package(PkgConfig QUIET)

# Resolves one external library and returns the target to link against.
function(hh_find_dependency out_var pc_name header lib_name)
    if(PkgConfig_FOUND)
        pkg_check_modules(${out_var} QUIET IMPORTED_TARGET GLOBAL ${pc_name})
        if(${out_var}_FOUND)
            set(${out_var} PkgConfig::${out_var} PARENT_SCOPE)
            return()
        endif()
    endif()

    find_path(${out_var}_INCLUDE_DIR NAMES ${header})
    find_library(${out_var}_LIBRARY NAMES ${lib_name})

    if(NOT ${out_var}_INCLUDE_DIR OR NOT ${out_var}_LIBRARY)
        message(FATAL_ERROR
            "Could not find ${pc_name}. Install its development package "
            "(libsodium-dev / libjpeg-dev on Debian and Ubuntu), or point "
            "CMAKE_PREFIX_PATH at your own build of it.")
    endif()

    add_library(${out_var}_imported UNKNOWN IMPORTED)
    set_target_properties(${out_var}_imported PROPERTIES
        IMPORTED_LOCATION "${${out_var}_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${${out_var}_INCLUDE_DIR}"
    )
    set(${out_var} ${out_var}_imported PARENT_SCOPE)
endfunction()

hh_find_dependency(HH_SODIUM libsodium sodium.h sodium)
hh_find_dependency(HH_JPEG   libjpeg  jpeglib.h jpeg)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

file(GLOB_RECURSE HH_VENDOR_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/third_party/*.c"
)

add_library(hh_core STATIC
    ${HH_CORE_SOURCES}
    ${HH_CORE_HEADERS}
    ${HH_VENDOR_SOURCES}
)
add_library(hushhush::core ALIAS hh_core)

set_target_properties(hh_core PROPERTIES
    OUTPUT_NAME hushhush
    POSITION_INDEPENDENT_CODE ON
)

# The parent of this directory is on the include path, so headers are reached
# as <core/...> both from inside the library and from its consumers.
target_include_directories(hh_core
    PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
    SYSTEM PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/third_party>"
)

# libsodium and libjpeg are PUBLIC: their headers are part of what core hands
# to its consumers, so the CLI gets the include paths without restating them.
target_link_libraries(hh_core
    PUBLIC
        hushhush::options
        Threads::Threads
        ${HH_SODIUM}
        ${HH_JPEG}
    PRIVATE
        $<$<NOT:$<PLATFORM_ID:Windows>>:m>
)

if(HH_VENDOR_SOURCES AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(${HH_VENDOR_SOURCES} PROPERTIES COMPILE_OPTIONS "-w")
endif()

if(HH_BUILD_TESTS)
    add_subdirectory(tests)
endif()
