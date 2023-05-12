#include "audio_source.h"
#include <iostream>
#include <cmath>
#include <cstring>
#include "circular_buffer.h"

static buffered_stream<float> audio_buffer{1024 * 30, 1024 * 20};

static int record_callback(void *outputBuffer, void *inputBuffer, unsigned int bufferFrames,
                           double streamTime, RtAudioStreamStatus status, void *userData) {
    if (status)
        std::cout << "Stream overflow detected!" << std::endl;

    auto *in = static_cast<float *>(inputBuffer);

    for (int i = 0; i < bufferFrames; i++) {
        audio_buffer.put(*in++);
        audio_buffer.put(*in++);
    }


    return 0;
}


static int render_callback(void *outputBuffer, void *, unsigned int bufferFrames, double, RtAudioStreamStatus, void *userData) {
    auto *out = static_cast<float *>(outputBuffer);

    if (audio_buffer.can_read()) {
        for (int i = 0; i < bufferFrames; i++) {
            *out++ = audio_buffer.get();
            *out++ = audio_buffer.get();
        }
    } else {
        memset(out, 0, sizeof(float) * 2 * bufferFrames);
    }

    return 0;
}

int get_input_device(RtAudio *audio) {
    auto devices = audio->getDeviceCount();

    for (size_t i = 0; i < devices; i++) {
        try {
            RtAudio::DeviceInfo info = audio->getDeviceInfo(i);
            auto pos = info.name.find("MS2109");

            if (pos != std::string::npos) {
                return i;
            }
        } catch (RtAudioError& e) {
            // ignore?
        }
    }

    return -1;
}


void enumerate_devices(RtAudio *audio) {
    std::cout << "Enumerating devices on interface" << std::endl << std::endl;
    // Determine the number of devices available
    auto device_count = audio->getDeviceCount();

    // Scan through devices for various capabilities
    for (int i = 0; i < device_count; i++) {
        try {
            auto info = audio->getDeviceInfo(i);

            if (info.probed) {
                std::cout << "device = " << info.name << std::endl;
                std::cout << "; device id = " << i << std::endl;
                std::cout << ": maximum output channels = " << info.outputChannels << "\n";
                std::cout << ": maximum input channels = " << info.inputChannels << "\n";
                std::cout << ": default output = " << info.isDefaultOutput << "\n";
                std::cout << ": default input = " << info.isDefaultInput << "\n";
                std::cout << ": samplerates = ";

                for (auto s: info.sampleRates)
                    std::cout << s << " ";

                std::cout << std::endl;

            }
        } catch (RtAudioError& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}

audio_source::audio_source() {
    try {
        audio_in = new RtAudio(RtAudio::LINUX_ALSA);
        audio_out = new RtAudio(RtAudio::LINUX_PULSE);
    } catch (RtAudioError& e) {
        std::cerr << "Failed to initialize RtAudio. Cause: " << e.what();
        exit(1);
    }

    std::cout << "Good, we're rolling" << std::endl;

    enumerate_devices(audio_in);
//    enumerate_devices(audio_out);

    // input stream
    try {
        RtAudio::StreamParameters iParams;
        iParams.deviceId = get_input_device(audio_in);
        iParams.nChannels = 2;
        iParams.firstChannel = 0;
        unsigned int sampleRate = 48000;
        unsigned int bufferFrames = 48;

        audio_in->openStream(nullptr, &iParams, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &record_callback, nullptr);
        audio_in->startStream();
    } catch (RtAudioError& e) {
        std::cerr << "Failed to open audio input stream. Cause: " << e.what();
    }


    // output stream
    int sampleRate = 48000;
    unsigned int bufferSize = 48;
    int nBuffers = 4;
    int device = 0;

    try {
        RtAudio::StreamParameters oParams;
        oParams.deviceId = audio_out->getDefaultOutputDevice();
        oParams.nChannels = 2;
        oParams.firstChannel = 0;

        audio_out->openStream(&oParams, nullptr, RTAUDIO_FLOAT32, sampleRate, &bufferSize, &render_callback, nullptr);
        audio_out->startStream();
    } catch (RtAudioError& e) {
        std::cerr << "Failed to open audio output stream. Cause: " << e.what();
    }

}

audio_source::~audio_source() {
    if (audio_out->isStreamOpen())
        audio_out->closeStream();

    if (audio_in->isStreamOpen())
        audio_in->closeStream();

    delete audio_out;
    delete audio_in;
}
