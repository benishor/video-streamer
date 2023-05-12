#include "streamer.h"

int main() {
    streamer stream("/dev/video3", 1280, 720);
    stream.loop();
    return 0;
}
