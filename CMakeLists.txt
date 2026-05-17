cmake_minimum_required(VERSION 3.22)
project(gtmodmenu VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── QPM extern path ──────────────────────────────────────────────────────────
set(EXTERN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern")

# Pull in QPM-managed headers & libs
include("${EXTERN_DIR}/beatsaber-hook/cmake/ndk-toolchain.cmake" OPTIONAL)

# ── Source files ─────────────────────────────────────────────────────────────
file(GLOB_RECURSE SOURCE_FILES
    "src/*.cpp"
    "src/**/*.cpp"
)

# ── Shared library target ─────────────────────────────────────────────────────
add_library(gtmodmenu SHARED ${SOURCE_FILES})

target_include_directories(gtmodmenu PRIVATE
    include
    src
    ${EXTERN_DIR}/beatsaber-hook/include
    ${EXTERN_DIR}/beatsaber-hook/shared
    ${EXTERN_DIR}/codegen/include
    ${EXTERN_DIR}/gorilla-utils/include
)

# ── Link QPM libraries ────────────────────────────────────────────────────────
target_link_libraries(gtmodmenu
    "-Wl,--exclude-libs,ALL"
    log
    android
)

# Glob any precompiled .so from extern
file(GLOB EXTERN_LIBS "${EXTERN_DIR}/libs/*.so")
if(EXTERN_LIBS)
    target_link_libraries(gtmodmenu ${EXTERN_LIBS})
endif()

# ── Compiler flags ────────────────────────────────────────────────────────────
target_compile_options(gtmodmenu PRIVATE
    -O2
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -DMOD_ID="gtmodmenu"
    -DVERSION="1.0.0"
)

# Output name used in mod.json
set_target_properties(gtmodmenu PROPERTIES
    OUTPUT_NAME "gtmodmenu"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
)
