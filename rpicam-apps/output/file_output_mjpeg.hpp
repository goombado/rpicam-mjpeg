#pragma once

#include "file_output.hpp"
#include "core/mjpeg_options.hpp"
#include "core/rpicam_mjpeg_encoder.hpp"

class FileOutputMJPEG : public FileOutput
{
public:
    FileOutputMJPEG(const MJPEGOptions *options, RPiCamMJPEGEncoder *encoder) : FileOutput((VideoOptions*) options), mjpeg_options_(options), encoder_(encoder) {}
    ~FileOutputMJPEG() {}

    protected:
        void closeFile() override;
    
    private:
        const MJPEGOptions *mjpeg_options_;
        RPiCamMJPEGEncoder *encoder_;
};