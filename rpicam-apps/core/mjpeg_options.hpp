#pragma once

#include <cstdio>

#include <string>
#include <ctime>

#include "video_options.hpp"

struct MJPEGOptions : public VideoOptions
{
    MJPEGOptions() : VideoOptions()
    {
        using namespace boost::program_options;

        /*
        Here we are adding in the options that were in the original RaspiMJPEG app.

        Look here: https://www.raspberrypi.com/documentation/computers/camera_software.html#differences-between-rpicam-and-raspicam 

        Documentation regarding porting of previous features from RaspiCAM: https://docs.google.com/document/d/19grw-U_GrXwHwHhGO39kM3aB3IjCONBWZC7nOWAJiBI/edit?usp=sharing

        Deprecater Features:
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
        ("output-image,om",value<std::string>(&output_image),
            "Set the output path for the MJPEG stream. Can be udp/tcp for network stream")
        ("output-preview,op",value<std::string>(&output_preview),
            "Set the output path for the low resolution preview stream. Can be udp/tcp for network stream")
        ("output-video,ov",value<std::string>(&output_video),
            "Set the output path for the video stream. Can be udp/tcp for network stream")
        ("raw-mode,rm",value<std::string>(&raw_mode_string),
			"Raw mode as W:H:bit-depth:packing, where packing is P (packed) or U (unpacked)")
        ("image-count,ic",value<unsigned int>(&image_count),
            "Set the image number for filename")
        ("video-count,vc",value<unsigned int>(&video_count),
            "Set the video number for filename")
        ;
    }

    std::string output_image;
    std::string output_preview;
    std::string output_video;
    std::string raw_mode_string;
    unsigned int image_count = 0;
    unsigned int video_count = 0;


    /*
    Notes:

    preview_path /dev/shm/mjpeg/cam.jpg
    image_path /var/www/media/im_%i_%Y%M%D_%h%m%s.jpg
    video_path /var/www/media/vi_%v_%Y%M%D_%h%m%s.mp4

     */

    virtual bool Parse(int argc, char *argv[]) override
    {

        // Testing
        // std::cout << "==================================================" << std::endl;

        if (output_preview.empty())
            {
                output_preview = "/dev/shm/mjpeg/cam.jpg";
                // Testing
                std::cout << output_preview << std::endl;
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

                // Testing
                std::cout << image_buffer << std::endl;

                free(image_buffer);
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

                // Testing
                std::cout << video_buffer << std::endl;

                free(video_buffer);
            }
        // raw_mode = Mode(raw_mode_string);


        // std::cout << "==================================================" << std::endl;

        return true;
    }

    virtual void Print() const override
    {
        VideoOptions::Print();
        std::cerr << "    output-mjpeg: " << output_image << std::endl;
        std::cerr << "    output-preview: " << output_preview << std::endl;
        std::cerr << "    output-video: " << output_video << std::endl;
    }
};