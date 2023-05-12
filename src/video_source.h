#pragma once

#include <thread>
#include <linux/videodev2.h>
#include "fps_counter.h"

struct video_buffer_info {
    void *start;
    size_t length;
};

class video_source {
public:
    video_source();
    video_source(const std::string& src, int w, int h, size_t buffer_count = 5);
    ~video_source();
    uint8_t *frame_buffer = nullptr;

private:
    uint32_t w, h;
    std::thread read_thread;
    volatile bool do_work;
    void read_fun();
    fd_set fds;
    timeval tv;
    int r, fd = -1;
    v4l2_buffer video_buffer;
    v4l2_format video_format;
    v4l2_requestbuffers buffer_request;
    video_buffer_info *buffers_info;
    size_t n_buffers;
    v4l2_buf_type buffer_type;
    fps_counter fps;
};