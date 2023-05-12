#include "streamer.h"

int main() {
    streamer e("/dev/video3", 1024, 768);
    e.loop();
    return 0;
}
