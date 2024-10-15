#pragma once

#include <filesystem>
#include <functional>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "core/rpicam_mjpeg_encoder.hpp"
#include "core/mjpeg_options.hpp"
#include "core/logging.hpp"

#include "image/image.hpp"

using namespace std::chrono_literals;
using namespace std::placeholders;
using libcamera::Stream;

namespace fs = std::filesystem;

class ImageSaver {
    public:
        ImageSaver(MJPEGOptions *options, const std::string camera_model) : options_((MJPEGOptions*) options), camera_model_(camera_model) {}

        std::string generate_filename()
        {
            char filename[128];
            std::string folder = options_->image_output; // sometimes "output" is used as a folder name
            if (!folder.empty() && folder.back() != '/')
                folder += "/";
            if (options_->datetime)
            {
                std::time_t raw_time;
                std::time(&raw_time);
                char time_string[32];
                std::tm *time_info = std::localtime(&raw_time);
                std::strftime(time_string, sizeof(time_string), "%m%d%H%M%S", time_info);
                snprintf(filename, sizeof(filename), "%s%s.%s", folder.c_str(), time_string, options_->encoding.c_str());
            }
            else if (options_->timestamp)
                snprintf(filename, sizeof(filename), "%s%u.%s", folder.c_str(), (unsigned)time(NULL),
                        options_->encoding.c_str());
            else
                snprintf(filename, sizeof(filename), options_->image_output.c_str(), options_->framestart);
            filename[sizeof(filename) - 1] = 0;
            return std::string(filename);
        }


        void update_latest_link(std::string const &filename)
        {
            // Create a fixed-name link to the most recent output file, if requested.
            if (!options_->latest.empty())
            {
                fs::path link { options_->latest };
                fs::path target { filename };

                if (fs::exists(link) && !fs::remove(link))
                    LOG_ERROR("WARNING: could not delete latest link " << options_->latest);
                else
                {
                    fs::create_symlink(target, link);
                    LOG(2, "Link " << options_->latest << " created");
                }
            }
        }




        void SaveImage(const std::vector<libcamera::Span<uint8_t>> &mem, CompletedRequestPtr &payload, StreamInfo info)
        {
            std::string filename = generate_filename();
            save_image(filename, mem, payload, info);
            update_latest_link(filename);
            options_->framestart++;
            if (options_->wrap)
                options_->framestart %= options_->wrap;
        }

    private:
        void save_image(std::string const &filename, const std::vector<libcamera::Span<uint8_t>> &mem, CompletedRequestPtr &payload, StreamInfo info)
        {
            if (options_->raw)
                dng_save(mem, info, payload->metadata, filename, camera_model_, (StillOptions*) options_);
            else if (options_->encoding == "jpg")
            {
                // a hack to make sure the quality attribute is correctly set
                // for the prewritten jpeg_save function
                jpeg_save(mem, info, payload->metadata, filename, camera_model_, (StillOptions*) options_);
            }
            else if (options_->encoding == "png")
                png_save(mem, info, filename, (StillOptions*) options_);
            else if (options_->encoding == "bmp")
                bmp_save(mem, info, filename, (StillOptions*) options_);
            else
                yuv_save(mem, info, filename, (StillOptions*) options_);
            LOG(2, "Saved image " << info.width << " x " << info.height << " to file " << filename);
        }

        void bayer_to_yuv(const std::vector<libcamera::Span<uint8_t>> &mem, StreamInfo info)
        {
            uint8_t *bayer_data = const_cast<uint8_t *>(mem[0].data());
            cv::Mat bayer(info.height, info.width, CV_8UC1, bayer_data);

            cv::Mat rgb;
            cv::cvtColor(bayer, rgb, cv::COLOR_);
            cv::Mat yuv;
            cv::cvtColor(rgb, yuv, cv::COLOR_RGB2YUV);
        }
        
        MJPEGOptions *options_;
        std::string const camera_model_;
};