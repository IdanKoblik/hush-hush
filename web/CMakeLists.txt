# The wasm module the page talks to. Emscripten only: everything here is
# meaningless under a native compiler, and `web/build.sh` is what drives it.
if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "web/ needs the Emscripten toolchain; run web/build.sh")
endif()

add_executable(hh_wasm wasm/hh_web.c)

set_target_properties(hh_wasm PROPERTIES
    OUTPUT_NAME hushhush
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dist"
    SUFFIX ".mjs"
)

target_link_libraries(hh_wasm PRIVATE hushhush::core)

# MODULARIZE with EXPORT_ES6 hands the page one factory to await, so the module
# is a plain <script type="module"> import with no global left behind. The
# runtime methods are the ones index.html reaches for: ccall for the entry
# points, FS to carry files in and out, UTF8ToString to read a returned string
# before hh_free takes it back. node is in ENVIRONMENT alongside web so
# wasm/smoke.mjs can drive the same artefact the page loads.
target_link_options(hh_wasm PRIVATE
    "-sMODULARIZE=1"
    "-sEXPORT_ES6=1"
    "-sEXPORT_NAME=createHushHush"
    "-sENVIRONMENT=web,node"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sINITIAL_MEMORY=64MB"
    "-sSTACK_SIZE=1MB"
    "-sEXIT_RUNTIME=0"
    "-sINVOKE_RUN=0"
    "-sFILESYSTEM=1"
    "-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,FS,UTF8ToString"
    "-sEXPORTED_FUNCTIONS=_hh_init,_hh_set_verbose,_hh_free,_hh_encode,_hh_decode,_hh_analyse,_malloc,_free"
)

# Argon2 through crypto_pwhash is the slowest thing the page does, and it runs
# on the main thread. -O3 is the difference between a pause and a hang.
target_compile_options(hh_wasm PRIVATE "-O3")
target_link_options(hh_wasm PRIVATE "-O3")
