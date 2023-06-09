cmake_minimum_required(VERSION 3.6)
project(streamer)

set(APP_NAME streamer)

find_package(PkgConfig REQUIRED)
pkg_search_module(SDL2 REQUIRED sdl2)
include_directories(${SDL2_INCLUDE_DIRS})

pkg_search_module(RTAUDIO REQUIRED rtaudio)
include_directories(${RTAUDIO_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wformat -g")
#set(CMAKE_BUILD_TYPE "Debug")

add_subdirectory(lib/glm EXCLUDE_FROM_ALL)
add_subdirectory(lib/cxxopts)
add_subdirectory(lib/v4l2-list-devices)

include_directories(src lib/glm/ lib/gl/include lib/cxxopts/include lib/v4l2-list-devices/include)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
add_executable(${APP_NAME} src/main.cpp lib/gl/src/glad.c src/fps_counter.cpp src/pbo.cpp src/streamer.cpp src/video_source.cpp src/audio_source.cpp)
target_link_libraries(
        ${APP_NAME}
        ${SDL2_LIBRARIES}
        ${RTAUDIO_LIBRARIES}
        GL v4l2)


