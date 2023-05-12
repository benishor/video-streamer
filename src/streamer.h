#pragma once

#include <string>
#include <array>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_opengl.h>

class pbo;
class video_source;

class streamer {
public:
    streamer(const std::string& device_path, int stream_width, int stream_height);
    ~streamer();

    void loop();

private:

    void toggle_fullscreen();
    bool is_fullscreen() const;

    int stream_width = 0;
    int stream_height = 0;

    std::array<int, 2> drawable_size = {0};
    std::array<int, 2> window_size = {0};
    bool update_viewport = false;

    pbo *pbo_ = nullptr;
    video_source *video = nullptr;
    SDL_Window* window = nullptr;
    SDL_GLContext* gl_context = nullptr;
};

