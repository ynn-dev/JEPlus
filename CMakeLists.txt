cmake_minimum_required(VERSION 3.28)

project(
    JEPlus
    VERSION 0.1.0
    LANGUAGES CXX
)

file(
    GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_library(JEPlus SHARED ${SOURCES})

target_compile_features(JEPlus PRIVATE cxx_std_23)

target_include_directories(JEPlus PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(JEPlus PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

set_target_properties(JEPlus PROPERTIES
    PREFIX ""
    SUFFIX ".asi"
    OUTPUT_NAME "JEPlus"
)

# https://github.com/cpm-cmake/CPM.cmake/wiki/Downloading-CPM.cmake-in-CMake
file(
    DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.43.2/CPM.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
    EXPECTED_HASH SHA256=49a3bef91ceb65bb66d57255e12d1ffd22abc2f6408fa9fe4534c544a2f232aa
)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

CPMAddPackage("gh:gabime/spdlog@1.17.0")
CPMAddPackage("gh:stephenberry/glaze@9.1.0")
CPMAddPackage("gh:cursey/safetyhook@0.7.0")

target_link_libraries(JEPlus PRIVATE
    spdlog::spdlog
    glaze::glaze
    safetyhook::safetyhook
)