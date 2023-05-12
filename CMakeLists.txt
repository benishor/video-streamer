cmake_minimum_required(VERSION 3.6)

set(APP_NAME streamer)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
include_directories(${GLFW_INCLUDE_DIRS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wformat -g")
#set(CMAKE_BUILD_TYPE "Debug")

file(GLOB src "src/*.h" "src/*.cpp")
file(GLOB gl3w "lib/gl3w/GL/*.h" "lib/gl3w/GL/*.c")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

# includes
add_subdirectory(lib/glm)
include_directories(src lib/gl3w lib/glm/)

# APP
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
add_executable(${APP_NAME} src/main.cpp src/fps_counter.cpp src/pbo.cpp src/streamer.cpp src/video_source.cpp lib/gl3w/GL/gl3w.c)
target_link_libraries(
        ${APP_NAME}
        ${GLFW_LIBRARIES}
        GL v4l2)


