#pragma once

#ifndef MJPEG_OPTIONS_HPP
#define MJPEG_OPTIONS_HPP

#include <cstdio>

#include <string>
#include <ctime>

#include "video_still_options.hpp"
#include "video_options.hpp"
#include "still_options.hpp"
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
        ("image-output,io",value<std::string>(&output_image)->default_value("/var/www/media/im_%i_%Y%m%d_%H%M%S.jpg"),
            "Set the output path for the MJPEG stream. Can be udp/tcp for network stream")
        ("preview-output,mo",value<std::string>(&output_preview)->default_value("/dev/shm/mjpeg/cam.jpg"),
            "Set the output path for the low resolution preview stream. Can be udp/tcp for network stream")
        ("video-output,vo",value<std::string>(&output_video)->default_value("/var/www/media/vi_%v_%Y%m%d_%H%M%S.mp4"),
            "Set the output path for the video stream. Can be udp/tcp for network stream")
        ("media-path,mp", value<std::string>(&media_path)->default_value("/var/www/media/"),
            "Set the base path for media files")
        ("image-width", value<unsigned int>(&image_width)->default_value(0),
            "Set the width of the image file")
        ("image-height", value<unsigned int>(&image_height)->default_value(0),
            "Set the height of the image file")
        ("image-mode", value<std::string>(&image_mode_string),
            "Camera mode for image as W:H:bit-depth:packing, where packing is P (packed) or U (unpacked)")
        ("image-stream-type", value<std::string>(&image_stream_type)->default_value("still"),
            "Sets the libcamera stream type for image capture. Can be \"raw\", \"still\", \"video\" or \"lores\". If \"video\" or \"lores\" is selected, the settings for their streams will be used for image capture, similar in principal to taking a screenshot of their respective streams.")
        ("image-raw-convert", value<bool>(&image_raw_convert)->default_value(false)->implicit_value(true),
            "If \"raw\" is selected for --image-stream-type, this flag will convert the raw image to the format specified by --encoding. If this flag is not set, the raw image will be saved as a DNG file")
        ("image-no-teardown", value<bool>(&image_no_teardown)->default_value(false)->implicit_value(true),
            "This will force the image stream to run simultaneously with video and lores streams. If --image-stream-type is \"video\" or \"lores\" this flag is enabled by default, and if it is \"still\" this flag cannot be enabled (stream type \"still\" can not run concurrently with video and lores streams and requires camera teardown)")
        ("video-capture-duration,vt", value<unsigned int>(&video_capture_duration)->default_value(0),
            "NOT WORKING: Sets video capture duration in seconds. If set to 0, video will keep recording until stopped.")
        ("video-split-interval,vi", value<unsigned int>(&video_split_interval)->default_value(0),
            "NOT WORKING: Sets video split interval in seconds. If set to 0, video will not be split.")
        ("control-file", value<std::string>(&control_file)->default_value("/var/www/FIFO"),
            "Sets the path to the named pipe for control commands. \"/var/www/FIFO\" is the default path")
        ("fifo-interval", value<unsigned int>(&fifo_interval)->default_value(100000),
            "Sets the interval in microseconds for the named pipe to be read from")
        ("motion-pipe", value<std::string>(&motion_pipe)->default_value("/var/www/FIFO1"),
            "Sets the path to the named pipe for motion detection commands. \"/var/www/FIFO1\" is the default path")
        ("ignore-etc-config", value<bool>(&ignore_etc_config)->default_value(false)->implicit_value(true),
            "Ignore the /etc/rpicam-mjpeg configuration file used by RPi_Cam_Web_Interface. If this flag is not set, the configuration file will be used if it exists. The --config option will override this flag.")
        ;
    }

    std::string output_image;
    std::string output_preview;
    std::string output_video;
    std::string raw_mode_string;
    std::string media_path;
    unsigned int image_width;
    unsigned int image_height;
    Mode image_mode;
    std::string image_mode_string;
    std::string image_stream_type;
    bool image_raw_convert;
    bool image_no_teardown;
    unsigned int video_capture_duration;
    unsigned int video_split_interval;
    std::string control_file;
    unsigned int fifo_interval;
    std::string motion_pipe;
    bool ignore_etc_config;


    /*
    Notes:

    preview_path /dev/shm/mjpeg/cam.jpg
    image_path /var/www/media/im_%i_%Y%M%D_%h%m%s.jpg
    video_path /var/www/media/vi_%v_%Y%M%D_%h%m%s.mp4
     */

    virtual bool Parse(int argc, char *argv[]) override
    {
        if (!lores_width)
            lores_width = 640;
        if (!lores_height)
            lores_height = 360;
        
        if (!image_width)
            image_width = 640;
        if (!image_height)
            image_height = 360;
        
        // check if the file /etc/rpicam-mjpeg exists, and if so set config_file to this
        // std::ifstream ifs("/etc/rpicam-mjpeg");
        // if (ifs and config_file.empty() and !ignore_etc_config)
        config_file = "/etc/rpicam-mjpeg";

        if (!VideoStillOptions::Parse(argc, argv))
            return false;

        image_mode = Mode(image_mode_string);

        // if output_preview doesn't end with .tmp, add .tmp
        if (output_preview.substr(output_preview.size() - 4) != ".tmp")
            output_preview += ".tmp";


        if (image_stream_type != "still" && image_stream_type != "raw" && image_stream_type != "video" && image_stream_type != "lores")
        {
            std::cerr << "Invalid image separate stream option: " << image_stream_type << std::endl;
            return false;
        }
        if (image_stream_type != "raw")
            no_raw = true;
        
        if (image_stream_type == "video" || image_stream_type == "lores")
            image_no_teardown = true;
        else if (image_stream_type == "still" && image_no_teardown)
        {
            std::cerr << "Cannot run still stream concurrently with video and lores streams" << std::endl;
            return false;
        }

        if (fifo_interval <= 0)
        {
            std::cerr << "Invalid FIFO interval" << std::endl;
            return false;
        }

        bool timeout_set = false;
        for (int i = 1; i < argc; ++i)
        {
            if (std::string(argv[i]) == "--timeout" || std::string(argv[i]) == "-t")
            {
            timeout_set = true;
            break;
            }
        }

        if (!timeout_set)
        {
            timeout.set("0");
        }
        
        nopreview = true;

        return true;
    }

    void Print() const override
    {
        VideoStillOptions::Print();
        std::cout << "    Image output: " << output_image << std::endl;
        std::cout << "    Video output: " << output_video << std::endl;
        std::cout << "    MJPEG output: " << output_preview << std::endl;
        std::cout << "    Media path: " << media_path << std::endl;
        std::cout << "    Image width: " << image_width << std::endl;
        std::cout << "    Image height: " << image_height << std::endl;
        std::cout << "    Image mode: " << image_mode_string << std::endl;
        std::cout << "    Image stream type: " << image_stream_type << std::endl;
        std::cout << "    Raw image convert: " << (image_raw_convert ? "true" : "false") << std::endl;
        std::cout << "    Image no teardown: " << (image_no_teardown ? "true" : "false") << std::endl;
        std::cout << "    Video capture duration: " << video_capture_duration << std::endl;
        std::cout << "    Video split interval: " << video_split_interval << std::endl;
        std::cout << "    Control file: " << control_file << std::endl;
        std::cout << "    FIFO interval: " << fifo_interval << std::endl;
        std::cout << "    Motion pipe: " << motion_pipe << std::endl;
        std::cout << "    Ignore /etc/rpicam-mjpeg: " << (ignore_etc_config ? "true" : "false") << std::endl;
        std::cout << "    Config File: " << config_file << std::endl;
    }

    StillOptions* GetStillOptions()
    {
        StillOptions* still_options = new StillOptions();
        still_options->quality = image_quality;
        still_options->exif = exif;
        still_options->timelapse = timelapse;
        still_options->framestart = framestart;
        still_options->datetime = datetime;
        still_options->timestamp = timestamp;
        still_options->restart = restart;
        still_options->keypress = keypress;
        still_options->signal = signal;
        still_options->thumb = thumb;
        still_options->thumb_width = thumb_width;
        still_options->thumb_height = thumb_height;
        still_options->thumb_quality = thumb_quality;
        still_options->encoding = encoding;
        still_options->raw = raw;
        still_options->latest = latest;
        still_options->zsl = zsl;

        return still_options;
    }

};

#endif // MJPEG_OPTIONS_HPP
