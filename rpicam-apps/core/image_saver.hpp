#pragma once

#include <filesystem>
#include <functional>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>


#include "core/rpicam_mjpeg_encoder.hpp"
#include "core/mjpeg_options.hpp"
#include "core/logging.hpp"
#include "image/image.hpp"
#include "image/dng.hpp"

using namespace std::chrono_literals;
using namespace std::placeholders;
using libcamera::Stream;

namespace fs = std::filesystem;


class ImageSaver {
    public:
        ImageSaver(MJPEGOptions *options, const std::string camera_model) : options_((MJPEGOptions*) options), camera_model_(camera_model) {}

        // std::string generate_filename()
        // {
        //     char filename[128];
        //     std::string folder = options_->image_output; // sometimes "output" is used as a folder name
        //     if (!folder.empty() && folder.back() != '/')
        //         folder += "/";
        //     if (options_->datetime)
        //     {
        //         std::time_t raw_time;
        //         std::time(&raw_time);
        //         char time_string[32];
        //         std::tm *time_info = std::localtime(&raw_time);
        //         std::strftime(time_string, sizeof(time_string), "%m%d%H%M%S", time_info);
        //         snprintf(filename, sizeof(filename), "%s%s.%s", folder.c_str(), time_string, options_->encoding.c_str());
        //     }
        //     else if (options_->timestamp)
        //         snprintf(filename, sizeof(filename), "%s%u.%s", folder.c_str(), (unsigned)time(NULL),
        //                 options_->encoding.c_str());
        //     else
        //         snprintf(filename, sizeof(filename), options_->image_output.c_str(), options_->framestart);
        //     filename[sizeof(filename) - 1] = 0;
        //     return std::string(filename);
        // }


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
            std::string filename = options_->output;
            save_image(filename, mem, payload, info);
            update_latest_link(filename);
            options_->framestart++;
            if (options_->wrap)
                options_->framestart %= options_->wrap;
            
            std::lock_guard<std::mutex> guard(active_threads_mutex_);
            active_threads_.emplace_back(dng_convert, options_, filename, info);
            active_threads_.back().detach();
        }

        void stop()
        {
            for (auto &thread : active_threads_)
                thread.join();
        }

    private:
        void save_image(std::string const &filename, const std::vector<libcamera::Span<uint8_t>> &mem, CompletedRequestPtr &payload, StreamInfo info)
        {
            // the below options come from rpicam_still
            // however they are not used in the image_saver, as rpicam-mjpeg only uses the raw stream for image capture
            // if (options_->raw)
            
            // set the dng filename to be the filename with the .dng extension
            std::string dng_filename = filename + ".dng";
            dng_save(mem, info, payload->metadata, dng_filename, camera_model_, (StillOptions*) options_);
            // else if (options_->encoding == "jpg")
            // {
            //     // a hack to make sure the quality attribute is correctly set
            //     // for the prewritten jpeg_save function
            //     jpeg_save(mem, info, payload->metadata, filename, camera_model_, (StillOptions*) options_);
            // }
            // else if (options_->encoding == "png")
            //     png_save(mem, info, filename, (StillOptions*) options_);
            // else if (options_->encoding == "bmp")
            //     bmp_save(mem, info, filename, (StillOptions*) options_);
            // else
            //     yuv_save(mem, info, filename, (StillOptions*) options_);
            LOG(2, "Saved image " << info.width << " x " << info.height << " to file " << filename);
        }

        static void run_command(std::string const &command)
        {
            if (system(command.c_str()) != 0)
                LOG_ERROR("WARNING: command failed: " << command);
        }

        static std::string const get_bayer_format_name(StreamInfo const &info)
        {
            auto it = bayer_formats.find(info.pixel_format);
            if (it == bayer_formats.end())
                throw std::runtime_error("unsupported Bayer format");
            BayerFormat const &bayer_format = it->second;

            return std::string(bayer_format.name);
        }

        static void dng_convert(const MJPEGOptions* options, std::string const &filename, StreamInfo const &info)
        {
            std::string dcraw_cmd = "dcraw -c -o 0 -q 0 ";
            std::string dng_filename = filename + ".dng";
            std::string bayer_name = get_bayer_format_name(info);
            if (bayer_name.find("16") != std::string::npos)
                dcraw_cmd += "-6 ";
            dcraw_cmd += dng_filename + " | ";

            std::string ppm_cmd = " >> " + filename;
            if (options->encoding == "ppm")
                ppm_cmd = "ppmtoppm " + ppm_cmd;
            else if (options->encoding == "jpg")
                ppm_cmd = "cjpeg -quality " + std::to_string(options->quality) + " -outfile " + filename;
            else if (options->encoding == "png")
                ppm_cmd = "pnmtopng " + ppm_cmd;
            else if (options->encoding == "bmp")
                ppm_cmd = "ppmtobmp " + ppm_cmd;
            else if (options->encoding == "rgb")
                ppm_cmd = "ppmtorgb " + ppm_cmd;
            else
                ppm_cmd = "ppmtoyuv " + ppm_cmd;
            
            run_command(dcraw_cmd + ppm_cmd);
            run_command("rm -f " + dng_filename);
            std::cout << "Finished saving image at " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() << std::endl;
        }
        
        MJPEGOptions *options_;
        std::string const camera_model_;
        std::vector<std::thread> active_threads_;
        std::mutex active_threads_mutex_;
};