cmake_minimum_required(VERSION 4.0.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
project(Scratchpad)


add_executable(Scratch)
target_sources(Scratch PRIVATE src/main.cpp src/console_colors.cpp)
target_link_libraries(Scratch PRIVATE "-lstdc++exp")

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