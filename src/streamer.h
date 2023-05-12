#pragma once

#include <string>
#include <array>
#include <SDL2/SDL_video.h>

class pbo;
class video_source;
class audio_source;

class streamer {
public:
    streamer(const std::string& video_device, const std::string& audio_device, int stream_width, int stream_height);
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
    audio_source *audio = nullptr;
    SDL_Window* window = nullptr;
    SDL_GLContext* gl_context = nullptr;
};

