#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <libv4l2.h>
#include "video_source.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>

#define CLEAR(x) memset(&(x), 0, sizeof(x))


static void xioctl(int fh, int request, void *arg) {
    int r;

    do {
        r = v4l2_ioctl(fh, request, arg);
    } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

    if (r == -1) {
        fprintf(stderr, "error %d, %s\\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

video_source::video_source(const std::string& src, int w_, int h_, size_t buffer_count_)
        : w(w_), h(h_), n_buffers(buffer_count_) {

    fd = v4l2_open(src.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        perror("Cannot open device");
        exit(EXIT_FAILURE);
    }

    CLEAR(video_format);

    video_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_format.fmt.pix.width = w;
    video_format.fmt.pix.height = h;
    video_format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    video_format.fmt.pix.field = V4L2_FIELD_INTERLACED;
    xioctl(fd, VIDIOC_S_FMT, &video_format);

    if (video_format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) {
        std::cerr << "libv4l didn't accept RGB24 format. Can't proceed." << std::endl;
        exit(EXIT_FAILURE);
    }

    if ((video_format.fmt.pix.width != w) || (video_format.fmt.pix.height != h)) {
        std::cout << "Warning: driver is sending image at "
                  << video_format.fmt.pix.width << "x" << video_format.fmt.pix.height
                  << std::endl;
    }

    CLEAR(buffer_request);
    buffer_request.count = n_buffers;
    buffer_request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer_request.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_REQBUFS, &buffer_request);

    buffers_info = new video_buffer_info[n_buffers];

    for (size_t i = 0; i < n_buffers; ++i) {
        CLEAR(video_buffer);

        video_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        video_buffer.memory = V4L2_MEMORY_MMAP;
        video_buffer.index = i;

        xioctl(fd, VIDIOC_QUERYBUF, &video_buffer);

        buffers_info[i].length = video_buffer.length;
        buffers_info[i].start = v4l2_mmap(nullptr, video_buffer.length,
                                          PROT_READ | PROT_WRITE, MAP_SHARED,
                                          fd, video_buffer.m.offset);

        if (MAP_FAILED == buffers_info[i].start) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }
    }

    for (size_t i = 0; i < n_buffers; ++i) {
        CLEAR(video_buffer);
        video_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        video_buffer.memory = V4L2_MEMORY_MMAP;
        video_buffer.index = i;
        xioctl(fd, VIDIOC_QBUF, &video_buffer);
    }

    buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMON, &buffer_type);

    do_work = true;
    std::thread th(&video_source::read_fun, this);
    swap(th, read_thread);
}

video_source::~video_source() {
    do_work = false;
    read_thread.join();

    buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMOFF, &buffer_type);

    for (size_t i = 0; i < n_buffers; ++i) {
        v4l2_munmap(buffers_info[i].start, buffers_info[i].length);
    }
    v4l2_close(fd);
}

void video_source::read_fun() {
    fps.start();
    while (do_work) {
        do {
            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            // timeout
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            r = select(fd + 1, &fds, nullptr, nullptr, &tv);
        } while ((r == -1 && (errno = EINTR)));

        if (r == -1) {
            perror("select");
            return;
        }

        CLEAR(video_buffer);
        video_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        video_buffer.memory = V4L2_MEMORY_MMAP;
        xioctl(fd, VIDIOC_DQBUF, &video_buffer);

        // TODO: should we copy or are we good the way we are?
        frame_buffer = static_cast<unsigned char *>(buffers_info[video_buffer.index].start);
        xioctl(fd, VIDIOC_QBUF, &video_buffer);

        fps.add_frame();
        if (fps.updated()) {
            std::cout << "Video source fps: " << fps.count() << std::endl;
            fps.reset();
        }
    }
}


