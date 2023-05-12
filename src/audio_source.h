#pragma once

#include <rtaudio/RtAudio.h>

class audio_source {
public:
    audio_source(const std::string& audio_device);
    ~audio_source();

    static void enumerate_input_devices();
    static void enumerate_output_devices();

private:
    RtAudio *audio_in;
    RtAudio *audio_out;
};