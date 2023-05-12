#pragma once

#include <rtaudio/RtAudio.h>

class audio_source {
public:
    audio_source();
    ~audio_source();

private:
    RtAudio *audio_in;
    RtAudio *audio_out;
};