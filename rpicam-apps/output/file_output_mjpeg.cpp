#include <filesystem>
#include "file_output_mjpeg.hpp"

void FileOutputMJPEG::closeFile()
{
    FileOutput::closeFile();
    encoder_->MoveTempMJPEGOutput();
}