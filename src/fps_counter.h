#pragma once

#include <cstddef>
#include <chrono>

class timer {
public:
    timer();

    float seconds_since_start();
    float lap();

private:
    std::chrono::time_point<std::chrono::system_clock> start_time;
    std::chrono::time_point<std::chrono::system_clock> last_lap_time;
};

class fps_counter {
public:
    void start() {
        reset();
    }


    void add_frame() {
        frames++;
    }

    bool updated() {
        elapsed_seconds += t.lap();
        return elapsed_seconds >= 1.0f;
    }

    size_t count() const {
        return frames;
    }

    void reset() {
        elapsed_seconds -= 1.0f;
        frames = 0;
    }

    size_t frames = 0;
    float elapsed_seconds = 0;
    timer t;

};
