#include <iostream>
#include <SDL.h>

#include "glad/glad.h"

#include "streamer.h"
#include "pbo.h"
#include "video_source.h"
#include "audio_source.h"

using namespace std;

streamer::streamer(const std::string& device_path, int w, int h) : stream_width(w), stream_height(h) {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to init SDL" << std::endl;
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    window = SDL_CreateWindow("streamer",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              stream_width,
                              stream_height,
                              flags);


    SDL_GL_GetDrawableSize(window, &drawable_size[0], &drawable_size[1]);
    std::cout << "Queried window drawable size: " << drawable_size[0] << "x" << drawable_size[1] << " (pixels)" << std::endl;

    SDL_GetWindowSize(window, &window_size[0], &window_size[1]);
    std::cout << "Queried window size: " << window_size[0] << "x" << window_size[1] << " (screen coords)" << std::endl;

    gl_context = static_cast<SDL_GLContext *>(SDL_GL_CreateContext(window));

    if (!gladLoadGL()) {
        std::cerr << "unable to load opengl functions!" << std::endl;
        exit(1);
    } else {
        std::cout << "created window and loaded opengl" << std::endl;
    }

    glEnable(GL_MULTISAMPLE);

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    pbo_ = new pbo(this);
    video = new video_source(device_path, stream_width, stream_height);
    audio = new audio_source();
}


streamer::~streamer() {
    delete audio;
    delete video;
    delete pbo_;
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}

void streamer::loop() {
    bool do_continue = true;
    while (do_continue) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        update_viewport = true;
                    }
                    break;
                case SDL_QUIT:
                    do_continue = false;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_f && event.key.type == SDL_KEYDOWN) {
                        toggle_fullscreen();
                    } else if (event.key.keysym.sym == SDLK_t && event.key.type == SDL_KEYDOWN) {
                        pbo_->toggle_texture_filtering();
                    }
                    break;
                default:
                    break;
            }
        }

        if (update_viewport) {
            int fb_width, fb_height;
            SDL_GL_GetDrawableSize(window, &fb_width, &fb_height);

            const float window_aspect_ratio = fb_width / static_cast<float>(fb_height);
            const float wanted_aspect_ratio = stream_width / static_cast<float>(stream_height);

            int new_width, new_height;

            if (wanted_aspect_ratio < window_aspect_ratio) {
                new_height = fb_height;
                new_width = (int) (new_height * wanted_aspect_ratio);
            } else {
                new_width = fb_width;
                new_height = (int) (new_width / wanted_aspect_ratio);
            }

            glClear(GL_COLOR_BUFFER_BIT);
            int new_xpos = (fb_width - new_width) / 2;
            int new_ypos = (fb_height - new_height) / 2;

            glViewport(new_xpos, new_ypos, new_width, new_height);

            update_viewport = false;
        }

        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        pbo_->fill(video->frame_buffer);
        pbo_->draw();

        SDL_GL_SwapWindow(window);
    }
//    while (!glfwWindowShouldClose(window)) {
//        glfwPollEvents();
//
//        if (update_viewport) {
//
//            int fb_width, fb_height;
//            glfwGetFramebufferSize(window, &fb_width, &fb_height);
//
//            const float window_aspect_ratio = fb_width / static_cast<float>(fb_height);
//            const float wanted_aspect_ratio = width / static_cast<float>(height);
//
//            int new_width, new_height;
//
//            if (wanted_aspect_ratio < window_aspect_ratio) {
//                new_height = fb_height;
//                new_width = (int) (new_height * wanted_aspect_ratio);
//            } else {
//                new_width = fb_width;
//                new_height = (int) (new_width / wanted_aspect_ratio);
//            }
//
//            glClear(GL_COLOR_BUFFER_BIT);
//            int new_xpos = (fb_width - new_width) / 2;
//            int new_ypos = (fb_height - new_height) / 2;
//
//            glViewport(new_xpos, new_ypos, new_width, new_height);
//
//            update_viewport = false;
//        }
//
//        glClearColor(0, 0, 0, 0);
//        glClear(GL_COLOR_BUFFER_BIT);
//
//        pbo_->fill(video->frame_buffer);
//        pbo_->draw();
//
//        glfwSwapBuffers(window);
//    }
}

bool streamer::is_fullscreen() const {
    auto flags = SDL_GetWindowFlags(window);
    return flags & SDL_WINDOW_FULLSCREEN;
}

void streamer::toggle_fullscreen() {
    if (is_fullscreen()) {
        SDL_SetWindowFullscreen(window, 0);
        // restore last window size and position
//        SDL_RestoreWindow(window); //Incase it's maximized...
//        SDL_SetWindowSize(screen, dm.w, dm.h + 10);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

        // restore last window size and position
//        glfwSetWindowMonitor(window, nullptr, window_pos[0], window_pos[1], window_size[0], window_size[1], 0);

    } else {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

//        // backup window position and window size
//        glfwGetWindowPos(window, &window_size[0], &window_pos[1]);
//        glfwGetWindowSize(window, &window_size[0], &window_pos[1]);
//
//        // get resolution of monitor
//        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
//
//        // switch to full screen
//        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, 0);
    }
    update_viewport = true;
}

//void streamer::on_window_resize(GLFWwindow *wnd, int w, int h) {
//    update_viewport = true;
//}
//
