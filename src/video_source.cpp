#include <cstring>
#include <string>
#include <sstream>
#include <libv4l2.h>
#include "video_source.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <iostream>
#include <map>
#include <sys/ioctl.h>
#include <vector>

#define CLEAR(x) memset(&(x), 0, sizeof(x))


static void xioctl(int fh, int request, void *arg) {
    int r;

    do {
        r = v4l2_ioctl(fh, request, arg);
    } while (r == -1 && (errno == EINTR || errno == EAGAIN));

    if (r == -1) {
        fprintf(stderr, "error %d, %s\\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}


#define MAP_ENTRY(x) {x, #x}

static std::string capabilities_to_string(uint32_t caps) {
    static std::map<uint32_t, std::string> cap_names = {
            MAP_ENTRY(V4L2_CAP_VIDEO_CAPTURE),
            MAP_ENTRY(V4L2_CAP_VIDEO_CAPTURE_MPLANE),
            MAP_ENTRY(V4L2_CAP_VIDEO_OUTPUT),
            MAP_ENTRY(V4L2_CAP_VIDEO_OUTPUT_MPLANE),
            MAP_ENTRY(V4L2_CAP_VIDEO_M2M),
            MAP_ENTRY(V4L2_CAP_VIDEO_M2M_MPLANE),
            MAP_ENTRY(V4L2_CAP_VIDEO_OVERLAY),
            MAP_ENTRY(V4L2_CAP_VBI_CAPTURE),
            MAP_ENTRY(V4L2_CAP_VBI_OUTPUT),
            MAP_ENTRY(V4L2_CAP_SLICED_VBI_CAPTURE),
            MAP_ENTRY(V4L2_CAP_SLICED_VBI_OUTPUT),
            MAP_ENTRY(V4L2_CAP_RDS_CAPTURE),
            MAP_ENTRY(V4L2_CAP_VIDEO_OUTPUT_OVERLAY),
            MAP_ENTRY(V4L2_CAP_HW_FREQ_SEEK),
            MAP_ENTRY(V4L2_CAP_RDS_OUTPUT),
            MAP_ENTRY(V4L2_CAP_TUNER),
            MAP_ENTRY(V4L2_CAP_AUDIO),
            MAP_ENTRY(V4L2_CAP_RADIO),
            MAP_ENTRY(V4L2_CAP_MODULATOR),
            MAP_ENTRY(V4L2_CAP_SDR_CAPTURE),
            MAP_ENTRY(V4L2_CAP_EXT_PIX_FORMAT),
            MAP_ENTRY(V4L2_CAP_SDR_OUTPUT),
            MAP_ENTRY(V4L2_CAP_READWRITE),
            MAP_ENTRY(V4L2_CAP_ASYNCIO),
            MAP_ENTRY(V4L2_CAP_STREAMING),
            MAP_ENTRY(V4L2_CAP_TOUCH),
            MAP_ENTRY(V4L2_CAP_DEVICE_CAPS),
    };

    std::stringstream ss;
    for (auto& kv: cap_names) {
        if (caps & kv.first) {
            ss << kv.second << " | ";
        }
    }

    return ss.str();
}

video_source::video_source(const std::string& src, int w_, int h_, size_t buffer_count_)
        : w(w_), h(h_), n_buffers(buffer_count_) {

    fd = v4l2_open(src.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        perror("Cannot open device");
        exit(EXIT_FAILURE);
    }

    v4l2_capability caps;
    CLEAR(caps);
    xioctl(fd, VIDIOC_QUERYCAP, &caps);
    std::cout << "Video capabilities:" << std::endl;
    std::cout << "\tDriver: " << caps.driver << std::endl;
    std::cout << "\tCard: " << caps.card << std::endl;
    std::cout << "\tBus info: " << caps.bus_info << std::endl;
    std::cout << "\tVersion: " << caps.version << std::endl;
    std::cout << "\tCaps: " << capabilities_to_string(caps.capabilities) << std::endl;
    std::cout << "\tDevice caps: " << capabilities_to_string(caps.device_caps) << std::endl;

    v4l2_streamparm sparams;
    CLEAR(sparams);
    sparams.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_G_PARM, &sparams);
    std::cout << "Stream params:" << std::endl;
    std::cout << "\tFPS: " << sparams.parm.capture.timeperframe.denominator << std::endl;


    // enumerate image formats
    v4l2_fmtdesc fmtdesc;
    CLEAR(fmtdesc);
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    std::vector<v4l2_fmtdesc> image_formats;

    std::cout << "Image formats" << std::endl;
    while (v4l2_ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        image_formats.push_back(fmtdesc);

        std::cout << "\tindex: " << fmtdesc.index << std::endl;
        std::cout << "\tdesc: " << fmtdesc.description << std::endl;
        std::cout << "\tpixel format: " << fmtdesc.pixelformat << std::endl;
        std::cout << std::endl;
        fmtdesc.index++;
    }


    // enumerate resolutions
    for (auto& image_fmt: image_formats) {
        v4l2_frmsizeenum frame_size;
        CLEAR(frame_size);
        frame_size.pixel_format = image_fmt.pixelformat;

        std::cout << "Resolution for pixel format " << image_fmt.description << std::endl;

        while (v4l2_ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frame_size) == 0) {
            std::cout << "\t" << frame_size.discrete.width << "x" << frame_size.discrete.height << std::endl;
            frame_size.index++;
        }

    }

    // enumerate video inputs
    int input;
    xioctl(fd, VIDIOC_G_INPUT, &input);
    std::cout << "Current input: " << input << std::endl;

    v4l2_input video_input;
    CLEAR(video_input);

    std::cout << "Video inputs" << std::endl;
    int r;
    while (true) {
        do {
            r = v4l2_ioctl(fd, VIDIOC_ENUMINPUT, &video_input);
        } while (r == -1 && (errno == EBUSY || errno == EAGAIN));

        if (r == -1 && EINVAL) break;

        std::cout << "\tindex: " << video_input.index << std::endl;
        std::cout << "\tname: " << video_input.name << std::endl;
        std::cout << "\ttype: " << video_input.type << std::endl;
        video_input.index++;
    }


    // -----------------------------------------------------------------------------------------------------------------
    // setting things
    // -----------------------------------------------------------------------------------------------------------------

    CLEAR(video_format);
    video_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_G_FMT, &video_format);


    video_format.fmt.pix.width = w;
    video_format.fmt.pix.height = h;
    video_format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    video_format.fmt.pix.field = V4L2_FIELD_ANY;
    xioctl(fd, VIDIOC_TRY_FMT, &video_format);

    xioctl(fd, VIDIOC_S_FMT, &video_format);

//    if (video_format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) {
//        std::cerr << "libv4l didn't accept RGB24 format. Can't proceed." << std::endl;
//        exit(EXIT_FAILURE);
//    }
//
//    if ((video_format.fmt.pix.width != w) || (video_format.fmt.pix.height != h)) {
//        std::cout << "Warning: driver is sending image at "
//                  << video_format.fmt.pix.width << "x" << video_format.fmt.pix.height
//                  << std::endl;
//    }

    // ask for buffers
    CLEAR(buffer_request);
    buffer_request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer_request.memory = V4L2_MEMORY_MMAP;
    buffer_request.count = n_buffers;
    xioctl(fd, VIDIOC_REQBUFS, &buffer_request);

    // query buffer info
    buffers_info = new video_buffer_info[n_buffers];

    for (size_t i = 0; i < n_buffers; ++i) {
        CLEAR(video_buffer);

        video_buffer.index = i;
        video_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        video_buffer.memory = V4L2_MEMORY_MMAP;

        xioctl(fd, VIDIOC_QUERYBUF, &video_buffer);

        buffers_info[i].offset = video_buffer.m.offset;
        buffers_info[i].length = video_buffer.length;
    }

    // memory map
    for (size_t i = 0; i < n_buffers; ++i) {
        buffers_info[i].start = v4l2_mmap(nullptr, buffers_info[i].length,
                                          PROT_READ | PROT_WRITE, MAP_SHARED,
                                          fd, buffers_info[i].offset);

        if (MAP_FAILED == buffers_info[i].start) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }
    }


    for (size_t i = 0; i < n_buffers; ++i) {
        video_buffer.index = i;
        video_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        video_buffer.memory = V4L2_MEMORY_MMAP;
        video_buffer.m.offset = buffers_info[i].offset;
        video_buffer.length = buffers_info[i].length;
        xioctl(fd, VIDIOC_QBUF, &video_buffer);
    }

    frame_buffer = new uint8_t[w * h * 3];


    buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMON, &buffer_type);

    v4l2_priority priority = V4L2_PRIORITY_RECORD;
    xioctl(fd, VIDIOC_S_PRIORITY, &priority);

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

        do {
            r = v4l2_ioctl(fd, VIDIOC_DQBUF, &video_buffer);
        } while (r == -1 && (errno == EBUSY || errno == EAGAIN));
        if (r == 0) {
            frame_buffer = static_cast<uint8_t *>(buffers_info[video_buffer.index].start);
        }
        xioctl(fd, VIDIOC_QBUF, &video_buffer);


        fps.add_frame();
        if (fps.updated()) {
            std::cout << "Video source fps: " << fps.count() << std::endl;
            fps.reset();
        }
    }
}


