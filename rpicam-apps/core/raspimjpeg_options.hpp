#pragma once

#include "video_options.hpp"

#include <cstdio>

#include <string>

struct RaspiMJPEGOptions : public VideoOptions
{
    RaspiMJPEGOptions() : VideoOptions()
    {
        using namespace boost::program_options;

        options_.add_options()
            ("output-mjpeg,om", value<std::string>(&output_mjpeg)->default_value(output + ".mjpeg"),
            "Set the output name for the MJPEG stream. Can be udp/tcp for network stream");
            ("output-preview,op", value<std::string>(&output_preview)->default_value("preview_" + output),
            "Set the output name for the low resolution preview stream. Can be udp/tcp for network stream");
        ;
    }

    virtual void Print() const override
    {
        VideoOptions::Print();
        std::cerr << "    output-mjpeg: " << output_mjpeg << std::endl;
        std::cerr << "    output-preview: " <<output_preview << std::endl;
    }

    std::string output_mjpeg;
    std::string output_preview;

};