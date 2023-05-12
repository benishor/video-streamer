#pragma once

#include <string>
#include <array>

class pbo;
class video_source;
class GLFWwindow;
class GLFWmonitor;

typedef unsigned int GLuint;

using namespace std;

class streamer {
public:
    streamer(const std::string& device_path, int w, int h);
    ~streamer();

    void loop();

private:

    void on_key_event(GLFWwindow *window, int key, int scancode, int action, int mods);
    void on_window_resize(GLFWwindow *wnd, int width, int height);
    void toggle_fullscreen();
    bool is_fullscreen() const;

    int width = 0;
    int height = 0;
    int window_x_pos = 0, window_y_pos = 0;
    int window_width = 0, window_height = 0;

    std::array<int, 2> window_pos = {0};
    std::array<int, 2> window_size = {0};
    std::array<int, 2> viewport_size = {0};
    bool update_viewport = false;

    GLFWwindow *window = nullptr;
    GLFWmonitor* monitor = nullptr;
    pbo *pbo_ = nullptr;
    video_source *video = nullptr;
};

