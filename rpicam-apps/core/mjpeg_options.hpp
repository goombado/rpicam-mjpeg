#pragma once

#ifndef MJPEG_OPTIONS_HPP
#define MJPEG_OPTIONS_HPP

#include <cstdio>

#include <string>
#include <ctime>

#include "video_still_options.hpp"
#include "core/options.hpp"

struct MJPEGOptions : public VideoStillOptions
{
    MJPEGOptions() : VideoStillOptions()
    {
        using namespace boost::program_options;

        /*
        Here we are adding in the options that were in the original RaspiMJPEG app.

        Look here: https://www.raspberrypi.com/documentation/computers/camera_software.html#differences-between-rpicam-and-raspicam 

        Documentation regarding porting of previous features from RaspiCAM: https://docs.google.com/document/d/19grw-U_GrXwHwHhGO39kM3aB3IjCONBWZC7nOWAJiBI/edit?usp=sharing

        Deprecated Features:
        - opacity (--opacity)
        - image effects (--imxfx)
        - colour effects (--colfx)
        - annotation (--annotate, --annotateex)
        - dynamic range compression, or DRC (--drc)
        - stereo (--stereo, --decimate and --3dswap)
        - image stabilisation (--vstab)
        - demo modes (--demo)
        - iso (--ISO)
        */
        options_.add_options()
        ("image-output,io",value<std::string>(&output_image),
            "Set the output path for the MJPEG stream. Can be udp/tcp for network stream")
        ("mjpeg-output,mo",value<std::string>(&output_preview),
            "Set the output path for the low resolution preview stream. Can be udp/tcp for network stream")
        ("video-output,vo",value<std::string>(&output_video),
            "Set the output path for the video stream. Can be udp/tcp for network stream")
        ("media-path", value<std::string>(&media_path)->default_value("/var/www/media/"),
             "Set the base path for media files")
        ("raw-mode,rm",value<std::string>(&raw_mode_string),
			"Raw mode as W:H:bit-depth:packing, where packing is P (packed) or U (unpacked)")
        ("image-count,ic",value<unsigned int>(&image_count),
            "Set the image number for filename")
        ("video-count,vc",value<unsigned int>(&video_count),
            "Set the video number for filename")
        ("image-width", value<unsigned int>(&image_width)->default_value(0),
             "Set the width of the image file")
        ("image-height", value<unsigned int>(&image_height)->default_value(0),
             "Set the height of the image file")
        ("image-mode", value<std::string>(&image_mode_string),
            "Camera mode for image as W:H:bit-depth:packing, where packing is P (packed) or U (unpacked)")
        ;
    }

    std::string output_image;
    std::string output_preview;
    std::string output_video;
    std::string raw_mode_string;
    unsigned int image_count = 0;
    unsigned int video_count = 0;
    std::string media_path;
    unsigned int image_width;
    unsigned int image_height;
    Mode image_mode;
    std::string image_mode_string;


    /*
    Notes:

    preview_path /dev/shm/mjpeg/cam.jpg
    image_path /var/www/media/im_%i_%Y%M%D_%h%m%s.jpg
    video_path /var/www/media/vi_%v_%Y%M%D_%h%m%s.mp4

     */

    virtual bool Parse(int argc, char *argv[]) override
    {
        if (!VideoStillOptions::Parse(argc, argv))
            return false;

        image_mode = Mode(image_mode_string);

        if (output_preview.empty())
            {
                output_preview = "/dev/shm/mjpeg/cam.jpg";
                // Testing
                // std::cout << output_preview << std::endl;
            }
              
        if (output_image.empty())
            {
                std::time_t t = std::time(nullptr);
                std::tm* now = std::localtime(&t);

                std::string sourceString = "/var/www/media/im_" + std::to_string(image_count) + "_%Y%m%d_%H%M%S.jpg";
                std::string bufferString = "/var/www/media/im__YYYYMMDD_HHMMSS.jpg";
                size_t image_buffer_size = bufferString.size()+std::to_string(image_count).length()+1;

                char* image_buffer = (char*)malloc(image_buffer_size);
                if (image_buffer == nullptr) {
                    std::cerr << "Memory allocation failed!" << std::endl;
                    return 1;
                }

                std::strftime(image_buffer, image_buffer_size, sourceString.c_str(), now);
                output_image = image_buffer;
                free(image_buffer);

                // Testing
                // std::cout << output_image << std::endl;
            }

        if (output_video.empty())
            {
                std::time_t t = std::time(nullptr);
                std::tm* now = std::localtime(&t);

                std::string sourceString = "/var/www/media/vi_" + std::to_string(video_count) + "_%Y%m%d_%H%M%S.mp4";
                std::string bufferString = "/var/www/media/vi__YYYYMMDD_HHMMSS.jpg";
                size_t video_buffer_size = bufferString.size()+std::to_string(video_count).length()+1;
                
                char* video_buffer = (char*)malloc(video_buffer_size);
                if (video_buffer == nullptr) {
                    std::cerr << "Memory allocation failed!" << std::endl;
                    return 1;
                }
                
                std::strftime(video_buffer, video_buffer_size, sourceString.c_str(), now);
                output_video = video_buffer;
                free(video_buffer);

                // Testing
                // std::cout << video_buffer << std::endl;
            }
        
        // raw_mode = Mode(raw_mode_string);

        return true;
    }

    void Print() const override
    {
        VideoStillOptions::Print();
        std::cout << "===============================================" << std::endl;
        std::cout << "Image output: " << output_image << std::endl;
        std::cout << "Video output: " << output_video << std::endl;
        std::cout << "MJPEG output: " << output_preview << std::endl;
        std::cout << "Media path: " << media_path << std::endl;
        std::cout << "Image width: " << image_width << std::endl;
        std::cout << "Image height: " << image_height << std::endl;
        std::cout << "Image mode: " << image_mode_string << std::endl;
    }
};

#endif // MJPEG_OPTIONS_HPP
