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
        ("output-mjpeg,om",value<std::string>(&output_mjpeg),
            "Set the output path for the MJPEG stream. Can be udp/tcp for network stream")
        ("output-preview,op",value<std::string>(&output_preview),
            "Set the output path for the low resolution preview stream. Can be udp/tcp for network stream")
        ("output-video,ov",value<std::string>(&output_video),
            "Set the output path for the video stream. Can be udp/tcp for network stream")
        ("raw-mode,rm",value<std::string>(&raw_mode_string),
			"Raw mode as W:H:bit-depth:packing, where packing is P (packed) or U (unpacked)");
        // ("sharpness,sh",value<);

        ;
    }

    std::string output_mjpeg;
    std::string output_preview;
    std::string output_video;
    std::string raw_mode_string;


    /*
    Notes:

    preview_path /dev/shm/mjpeg/cam.jpg
    image_path /var/www/media/im_%i_%Y%M%D_%h%m%s.jpg
    video_path /var/www/media/vi_%v_%Y%M%D_%h%m%s.mp4
     */

    virtual bool Parse(int argc, char *argv[]) override
    {

        // Testing
        std::cout << "==================================================" << std::endl;

        if (output_preview.empty())
            output_preview = "/dev/shm/mjpeg/cam.jpg";
        if (output_mjpeg.empty())
            output_mjpeg = "test.mjpeg";
        if (output_video.empty())
            {
                // Get current time and local time
                std::time_t t = std::time(nullptr);
                std::tm* now = std::localtime(&t);
                // char buffer[100];
                char* buffer = (char*) malloc(sizeof(char)*sizeof("/var/www/media/vi_%Y%m%d_%H%M%S.jpg"));
                std::strftime(buffer, sizeof(buffer), "/var/www/media/vi_%Y%m%d_%H%M%S.jpg", now);
                std::cout << buffer << std::endl;
            }
        // raw_mode = Mode(raw_mode_string);


        std::cout << "==================================================" << std::endl;

        return true;
    }

    virtual void Print() const override
    {
        VideoOptions::Print();
        std::cerr << "    output-mjpeg: " << output_mjpeg << std::endl;
        std::cerr << "    output-preview: " << output_preview << std::endl;
        std::cerr << "    output-video: " << output_video << std::endl;
    }
};