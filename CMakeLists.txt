cmake_minimum_required(VERSION 4.2...4.5)

project(Scratchpad
    VERSION "0.0.1"
    DESCRIPTION
    "Project for authors experimentation with various features and libraries"
    LANGUAGES CXX)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

include(CMake_dependencies.cmake)

add_executable(Scratch)
target_sources(Scratch PRIVATE src/main.cpp)

target_link_libraries(Scratch PRIVATE "-lstdc++exp")
target_link_libraries(Scratch PRIVATE sqlpp23::sqlpp23 sqlpp23::sqlite3)

target_compile_features(Scratch PRIVATE cxx_std_26)
target_compile_options(Scratch PRIVATE -freflection)

if(MINGW AND (CMAKE_BUILD_TYPE STREQUAL "Release"))
    target_link_options(Scratch PUBLIC -static -static-libgcc -static-libstdc++)
endif()

target_sources(Scratch
    PUBLIC
    FILE_SET pub_headers
    TYPE HEADERS
    BASE_DIRS
    include)

target_sources(Scratch
    PRIVATE
    FILE_SET priv_headers
    TYPE HEADERS
    BASE_DIRS
    src/private_inc)
