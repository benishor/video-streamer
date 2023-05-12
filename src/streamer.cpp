#include <stdio.h>
#include <iostream>
#include <stdexcept>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <functional>

#include "streamer.h"
#include "pbo.h"
#include "video_source.h"

using namespace std;

static void error_callback(int error, const char *description) {
    std::cerr << "Error " << error << ": " << description << std::endl;
}

void streamer::on_key_event(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        toggle_fullscreen();
    }

    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        pbo_->toggle_texture_filtering();
    }
}

streamer::streamer(const std::string& device_path, int w, int h) : width(w), height(h) {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        throw std::runtime_error("Can't init glfw context!");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

//    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    window = glfwCreateWindow(width, height, "BenStreamer", nullptr, nullptr);
    if (window == nullptr) {
        glfwTerminate();
        throw std::runtime_error("Failed to create window!");
    }

    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);

    monitor = glfwGetPrimaryMonitor();
    glfwGetWindowSize(window, &window_size[0], &window_size[1]);
    glfwGetWindowPos(window, &window_pos[0], &window_pos[1]);
    update_viewport = true;

    glfwSwapInterval(1); // Enable vsync
    gl3wInit();

    glfwSetWindowSizeCallback(window, [](GLFWwindow *wnd, int w, int h) {
        auto self = static_cast<streamer *>(glfwGetWindowUserPointer(wnd));
        self->on_window_resize(wnd, w, h);
    });
    glfwSetKeyCallback(window, [](GLFWwindow *wnd, int key, int scancode, int action, int mods) {
        auto self = static_cast<streamer *>(glfwGetWindowUserPointer(wnd));
        self->on_key_event(wnd, key, scancode, action, mods);
    });

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    pbo_ = new pbo(this);
    video = new video_source(device_path, width, height);
}


streamer::~streamer() {
    delete video;
    delete pbo_;

    glfwDestroyWindow(window);
    glfwTerminate();
}

void streamer::loop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (update_viewport) {

            int fb_width, fb_height;
            glfwGetFramebufferSize(window, &fb_width, &fb_height);

            const float window_aspect_ratio = fb_width / static_cast<float>(fb_height);
            const float wanted_aspect_ratio = width / static_cast<float>(height);

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

        glfwSwapBuffers(window);
    }
}

bool streamer::is_fullscreen() const {
    return glfwGetWindowMonitor(window) != nullptr;
}

void streamer::toggle_fullscreen() {

    if (is_fullscreen()) {
        // restore last window size and position
        glfwSetWindowMonitor(window, nullptr, window_pos[0], window_pos[1], window_size[0], window_size[1], 0);

    } else {
        // backup window position and window size
        glfwGetWindowPos(window, &window_size[0], &window_pos[1]);
        glfwGetWindowSize(window, &window_size[0], &window_pos[1]);

        // get resolution of monitor
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);

        // switch to full screen
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, 0);
    }
}

void streamer::on_window_resize(GLFWwindow *wnd, int w, int h) {
    update_viewport = true;
}

