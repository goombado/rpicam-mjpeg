// placeholder for MJPEG options
#pragma once

#ifndef MJPEG_OPTIONS_HPP
#define MJPEG_OPTIONS_HPP

#include <cstdio>
#include <string>

#include "video_still_options.hpp"
#include "core/options.hpp"

struct MJPEGOptions : public VideoStillOptions {
    MJPEGOptions() : VideoStillOptions() {

        using namespace boost::program_options;

        options_.add_options()
            ("image-output,io", value<std::string>(&image_output)->default_value("/var/www/media/capture.JPG"),
             "Set the output name for the image file")
            ("video-output,vo", value<std::string>(&video_output)->default_value("/var/www/media/capture.MP4"),
             "Set the output name for the video file")
            ("mjpeg-output,mo", value<std::string>(&mjpeg_output)->default_value("/dev/shm/mjpeg/cam.jpg"),
             "Set the output name for the MJPEG preview file")
            ("media-path", value<std::string>(&media_path)->default_value("/var/www/media/"),
             "Set the base path for media files")
            ("image-width", value<unsigned int>(&image_width)->default_value(0),
             "Set the width of the image file")
            ("image-height", value<unsigned int>(&image_height)->default_value(0),
             "Set the height of the image file")
            ("image-mode", value<std::string>(&image_mode_string),
             "Camera mode for image as W:H:bit-depth:packing, where packing is P (packed) or U (unpacked)")
        ;
    }

    void Print() const override {
        VideoStillOptions::Print();
        std::cout << "Image output: " << image_output << std::endl;
        std::cout << "Video output: " << video_output << std::endl;
        std::cout << "MJPEG output: " << mjpeg_output << std::endl;
        std::cout << "Media path: " << media_path << std::endl;
        std::cout << "Image width: " << image_width << std::endl;
        std::cout << "Image height: " << image_height << std::endl;
        std::cout << "Image mode: " << image_mode_string << std::endl;
    }

    bool Parse(int argc, char *argv[]) override {
        if (!VideoStillOptions::Parse(argc, argv))
            return false;

        image_mode = Mode(image_mode_string);

        return true;
    }

    std::string image_output;
    std::string video_output;
    std::string mjpeg_output;
    std::string media_path;
    unsigned int image_width;
    unsigned int image_height;
    Mode image_mode;
    std::string image_mode_string;
};

#endif // MJPEG_OPTIONS_HPP