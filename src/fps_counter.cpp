#include "fps_counter.h"

timer::timer() {
    start_time = std::chrono::system_clock::now();
    last_lap_time = start_time;
}

float timer::seconds_since_start() {
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::chrono::duration<float> elapsed_seconds = now - start_time;
    return elapsed_seconds.count();
}

float timer::lap() {
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::chrono::duration<float> elapsed_seconds = now - last_lap_time;
    last_lap_time = now;
    return elapsed_seconds.count();
}


