#include <iostream>
#include "streamer.h"
#include "cxxopts.hpp"
#include "string_utils.h"
#include "audio_source.h"
#include "video_source.h"

int main(int argc, char **argv) {
    cxxopts::Options options("streamer", "A video/audio streamer for v4l2 devices");
    options.positional_help("[list-video] [list-audio]")
            .show_positional_help();

    options.add_options()
            ("v,video-device", "The video device", cxxopts::value<std::string>()->default_value("/dev/video1"))
            ("a,audio-device", "The ALSA audio device", cxxopts::value<std::string>()->default_value("default"))
            ("g,geometry", "Desired stream resolution in pixels WxH", cxxopts::value<std::string>()->default_value("800x600"))
            ("h,help", "Print usage");

    options.allow_unrecognised_options();

    auto result = options.parse(argc, argv);
    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    // TODO: surely there must be a better way for optional positional args? find it
    if (!result.unmatched().empty()) {
        if (result.unmatched().front() == "list-video") {
            std::cout << "Enumerating video devices" << std::endl << std::endl;
            video_source::enumerate_video_devices();
            return 0;
        }

        if (result.unmatched().front() == "list-audio") {
            std::cout << "Enumerating audio devices" << std::endl << std::endl;
            audio_source::enumerate_input_devices();
            return 0;
        }
    }

    auto video_device = result["video-device"].as<std::string>();
    auto geometry = result["geometry"].as<std::string>();
    auto pieces = split_string(geometry, "x");
    auto width = std::stoi(pieces[0]);
    auto height = pieces.size() > 1 ? std::stoi(pieces[1]) : 0;

    auto audio_device = result["audio-device"].as<std::string>();

    streamer stream(video_device, audio_device, width, height);
    stream.loop();
    return 0;
}
