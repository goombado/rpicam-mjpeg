#pragma once

#include <cstdio>

#include <string>

#include "video_options.hpp"

struct MjpegOptions : public VideoOptions
{
    MjpegOptions() : VideoOptions()
    {
        using namespace boost::program_options;

        options_.add_options()
        ("output-mjpeg,om",value<std::string>(&output_mjpeg)->default_value(0),
            "Set the output name for the MJPEG stream. Can be udp/tcp for network stream");
        ("output-preview,op",value<std::string>(&output_preview)->default_value(0),
            "Set the output name for the low resolution preview stream. Can be udp/tcp for network stream");
        ("output-video,ov",value<std::string>(&output_video)->default_value(0),
            "Set the output name for the video stream. Can be udp/tcp for network stream");
    }

    std::string output_mjpeg;
    std::string output_preview;
    std::string output_video;

    virtual bool Parse(int argc, char* argv[]) override
    {
        if (VideoOptions::Parse(argc, argv) == false)
            return false;

        if (output_mjpeg.empty())
            output_mjpeg = "test.mjpeg";
        if (output_preview.empty())
            output_preview = "preview.mp4";
        if (output_video.empty())
            output_video = "video.mp4";

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