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
        ("mjpeg-output,mo",value<std::string>(&output_preview)->default_value("/dev/shm/mjpeg/cam.jpg"),
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
            "Sets video capture duration in seconds. If set to 0, video will keep recording until stopped.")
        ("video-split-interval,vi", value<unsigned int>(&video_split_interval)->default_value(0),
            "Sets video split interval in seconds. If set to 0, video will not be split.")
        ("control-file", value<std::string>(&control_file)->default_value("/var/www/FIFO"),
            "Sets the path to the named pipe for control commands. \"/var/www/FIFO\" is the default path")
        ("fifo-interval", value<unsigned int>(&fifo_interval)->default_value(100000),
            "Sets the interval in microseconds for the named pipe to be read from")
        ("motion-pipe", value<std::string>(&motion_pipe)->default_value("/var/www/FIFO1"),
            "Sets the path to the named pipe for motion detection commands. \"/var/www/FIFO1\" is the default path")
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

    void ReconstructArgs(std::vector<std::string> &args) const override
    {
        VideoStillOptions::ReconstructArgs(args);

        if (!output_image.empty())
            args.push_back("--image-output=" + output_image);
        if (!output_preview.empty())
            args.push_back("--mjpeg-output=" + output_preview);
        if (!output_video.empty())
            args.push_back("--video-output=" + output_video);
        if (!media_path.empty())
            args.push_back("--media-path=" + media_path);
        if (image_width != 0)
            args.push_back("--image-width=" + std::to_string(image_width));
        if (image_height != 0)
            args.push_back("--image-height=" + std::to_string(image_height));
        if (!image_mode_string.empty())
            args.push_back("--image-mode=" + image_mode_string);
        if (!image_stream_type.empty())
            args.push_back("--image-stream-type=" + image_stream_type);
        if (image_raw_convert)
            args.push_back("--image-raw-convert");
        if (image_no_teardown)
            args.push_back("--image-no-teardown");
        if (video_capture_duration != 0)
            args.push_back("--video-capture-duration=" + std::to_string(video_capture_duration));
        if (video_split_interval != 0)
            args.push_back("--video-split-interval=" + std::to_string(video_split_interval));
        if (!control_file.empty())
            args.push_back("--control-file=" + control_file);
        if (fifo_interval != 0)
            args.push_back("--fifo-interval=" + std::to_string(fifo_interval));
        if (!motion_pipe.empty())
            args.push_back("--motion-pipe=" + motion_pipe);
        
        return;
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
