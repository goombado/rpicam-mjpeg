/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * rpicam_encoder.cpp - libcamera video encoding class.
 */

#pragma once

#include <string>
#include <filesystem>

#include <dirent.h>
#include <time.h>
#include <ctime>

#include "core/rpicam_app.hpp"
#include "core/stream_info.hpp"
#include "core/mjpeg_options.hpp"
#include "core/image_saver.hpp"

#include "encoder/encoder.hpp"

#define MAX_UNSIGNED_INT_LENGTH 10 // 4,294,967,295
#define NULL_TERMINATOR_LENGTH 1
#define FILE_PLACEHOLDER_LENGTH 2 // %v or %i

typedef std::function<void(void *, size_t, int64_t, bool)> EncodeOutputReadyCallback;
typedef std::function<void(libcamera::ControlList &)> MetadataReadyCallback;

class RPiCamMJPEGEncoder : public RPiCamApp
{
public:
	using Stream = libcamera::Stream;
	using FrameBuffer = libcamera::FrameBuffer;

	RPiCamMJPEGEncoder() : RPiCamApp(std::make_unique<MJPEGOptions>()) {}

	void ConfigureMJPEG()
	{
		LOG(2, "Configuring MJPEG...");

		// Stream 1: H264 Video Recording Stream
		// Stream 2: Lores stream for MJPEG and motion detection post-processing
		// Stream 3: image_stream -> RAW stream for image capture
		StreamRoles stream_roles = { StreamRole::VideoRecording, StreamRole::Viewfinder, StreamRole::Raw };
		configuration_ = camera_->generateConfiguration(stream_roles);

		if (!configuration_)
			throw std::runtime_error("failed to generate MJPEG configuration");

		
		
		// Now we get to override any of the default settings from the options->
		MJPEGOptions *options = GetOptions();

		StreamConfiguration &cfg = configuration_->at(0);
		cfg.pixelFormat = libcamera::formats::YUV420;
		cfg.bufferCount = 6; // 6 buffers is better than 4
		if (options->buffer_count > 0)
			cfg.bufferCount = options->buffer_count;
		if (options->width)
			cfg.size.width = options->width;
		if (options->height)
			cfg.size.height = options->height;

		// VideoRecording stream should be in Rec709 color space
		if (cfg.size.width >= 1280 || cfg.size.height >= 720)
			cfg.colorSpace = libcamera::ColorSpace::Rec709;
		else
			cfg.colorSpace = libcamera::ColorSpace::Smpte170m;
		configuration_->orientation = libcamera::Orientation::Rotate0 * options->transform;

		post_processor_.AdjustConfig("video", &configuration_->at(0));

		// next configure the lores Viewfinder stream
		// we want to make this similar to the current rpicam-apps lores stream

		Size lores_size(options->lores_width, options->lores_height);
		lores_size.alignDownTo(2, 2);
		if (lores_size.width > configuration_->at(0).size.width ||
			lores_size.height > configuration_->at(0).size.height)
			throw std::runtime_error("Low resolution stream larger than video");
		configuration_->at(1).pixelFormat = lores_format_;
		configuration_->at(1).size = lores_size;
		configuration_->at(1).bufferCount = configuration_->at(0).bufferCount;
		configuration_->at(1).colorSpace = libcamera::ColorSpace::Sycc;

		// TODO: CHANGE TO RAW MODE
		// currently this sets RAW to be configured the same as the VideoRecording stream
		// but we want RAW to be configured to be the highest resolution possible (probably?)
		// or to the manually supplied (not yet existing) raw_mode
		// for now we will assume that images will always be taken at the highest resolution
		// and later add a raw_mode option to the options class
		options->mode.update(configuration_->at(0).size, options->framerate);
		options->image_mode.update(Size(options->image_width, options->image_height), options->framerate);
		options->image_mode = selectMode(options->image_mode);

		configuration_->at(2).size = options->image_mode.Size();
		configuration_->at(2).pixelFormat = mode_to_pixel_format(options->image_mode);
		configuration_->sensorConfig = libcamera::SensorConfiguration();
		configuration_->sensorConfig->outputSize = options->image_mode.Size();
		configuration_->sensorConfig->bitDepth = options->image_mode.bit_depth;
		configuration_->at(2).bufferCount = configuration_->at(0).bufferCount;


		configureDenoise(options->denoise == "auto" ? "cdn_fast" : options->denoise);
		setupCapture();

		streams_["video"] = configuration_->at(0).stream();
		streams_["lores"] = configuration_->at(1).stream();
		streams_["raw"] = configuration_->at(2).stream();

		post_processor_.Configure();

		LOG(2, "MJPEG setup complete");

	}

	/*
	InitialiseCount checks for the existence of a count.txt file in the media_path/.rpicam-mjpeg folder
	If the file exists, it reads the video_count and image_count from the file
	If the file does not exist, it creates the file and writes the video_count and image_count to the file as 0.
	*/ 
	void InitialiseCount()
	{
		createMediaPath();
		std::filesystem::path media_path = GetOptions()->media_path;
		std::filesystem::path config_folder = media_path / ".rpicam-mjpeg";

		if (std::filesystem::exists(config_folder) && std::filesystem::is_directory(media_path))
			;
		else
			std::filesystem::create_directory(config_folder);
		
		std::filesystem::path item_count_file = config_folder / "count.txt";
		if (std::filesystem::exists(item_count_file))
		{
			std::ifstream file(item_count_file);
			file >> video_count >> image_count;
			file.close();
		}
		else
		{
			// create the file
			std::ofstream file(item_count_file);
			file << video_count << std::endl;
			file << image_count << std::endl;
			file.close();
		}
	}

	/*
	SaveCount writes the video_count and image_count to the count.txt file in the media_path/.rpicam-mjpeg folder
	*/
	void SaveCount()
	{
		std::filesystem::path media_path = GetOptions()->media_path;
		std::filesystem::path config_folder = media_path / ".rpicam-mjpeg";
		std::filesystem::path item_count_file = config_folder / "count.txt";
		std::ofstream file(item_count_file);
		file << video_count << std::endl;
		file << image_count << std::endl;
		file.close();
	}

	void StartEncoders()
	{
		StartVideoEncoder();
		StartLoresEncoder();
		StartImageSaver();
	}

	void StartVideoEncoder()
	{
		createVideoEncoder();
		video_encoder_->SetInputDoneCallback(std::bind(&RPiCamMJPEGEncoder::videoEncodeBufferDone, this, std::placeholders::_1));
		video_encoder_->SetOutputReadyCallback(video_encode_output_ready_callback_);
	}

	void StartLoresEncoder()
	{
		createLoresEncoder();
		lores_encoder_->SetInputDoneCallback(std::bind(&RPiCamMJPEGEncoder::loresEncodeBufferDone, this, std::placeholders::_1));
		lores_encoder_->SetOutputReadyCallback(lores_encode_output_ready_callback_);
	}

	void StartImageSaver()
	{
		createImageSaver();
	}

	// This is callback when the encoder gives you the encoded output data.
	void SetVideoEncodeOutputReadyCallback(EncodeOutputReadyCallback callback) { video_encode_output_ready_callback_ = callback; }
	void SetLoresEncodeOutputReadyCallback(EncodeOutputReadyCallback callback) { lores_encode_output_ready_callback_ = callback; }
	void SetVideoMetadataReadyCallback(MetadataReadyCallback callback) { video_metadata_ready_callback_ = callback; };
	void SetLoresMetadataReadyCallback(MetadataReadyCallback callback) { lores_metadata_ready_callback_ = callback; };

	void VideoEncodeBuffer(CompletedRequestPtr &completed_request, Stream *stream)
	{
		assert(video_encoder_);
		StreamInfo info = GetStreamInfo(stream);
		FrameBuffer *buffer = completed_request->buffers[stream];
		BufferReadSync r(this, buffer);
		libcamera::Span span = r.Get()[0];
		void *mem = span.data();
		if (!buffer || !mem)
			throw std::runtime_error("no buffer to encode");
		auto ts = completed_request->metadata.get(controls::SensorTimestamp);
		int64_t timestamp_ns = ts ? *ts : buffer->metadata().timestamp;
		{
			std::lock_guard<std::mutex> lock(encode_buffer_queue_mutex_);
			encode_buffer_queue_.push(completed_request); // creates a new reference
		}
		video_encoder_->EncodeBuffer(buffer->planes()[0].fd.get(), span.size(), mem, info, timestamp_ns / 1000);
	}

	void LoresEncodeBuffer(CompletedRequestPtr &completed_request, Stream *stream)
	{
		assert(lores_encoder_);
		StreamInfo info = GetStreamInfo(stream);
		FrameBuffer *buffer = completed_request->buffers[stream];
		BufferReadSync r(this, buffer);
		libcamera::Span span = r.Get()[0];
		void *mem = span.data();
		if (!buffer || !mem)
			throw std::runtime_error("no buffer to encode");
		auto ts = completed_request->metadata.get(controls::SensorTimestamp);
		int64_t timestamp_ns = ts ? *ts : buffer->metadata().timestamp;
		{
			std::lock_guard<std::mutex> lock(encode_buffer_queue_mutex_);
			encode_buffer_queue_.push(completed_request); // creates a new reference
		}
		lores_encoder_->EncodeBuffer(buffer->planes()[0].fd.get(), span.size(), mem, info, timestamp_ns / 1000);
	}

	void SaveImage(CompletedRequestPtr &completed_request, Stream *stream)
	{
		StreamInfo info = GetStreamInfo(stream);
		FrameBuffer *buffer = completed_request->buffers[stream];
		BufferReadSync r(this, buffer);
		const std::vector<libcamera::Span<uint8_t>> mem = r.Get();
		if (!buffer || mem.empty())
			throw std::runtime_error("no buffer to encode");

		MJPEGOptions *image_options = (MJPEGOptions* ) GetImageOptions();

		makeFilename(&(image_options->output), image_options->output_image);
		image_saver_->SaveImage(mem, completed_request, info);

		image_count++;
	}

	MJPEGOptions *GetOptions() const { return static_cast<MJPEGOptions *>(options_.get()); }
	MJPEGOptions *GetOptions() { return static_cast<MJPEGOptions *>(options_.get()); }
	MJPEGOptions *GetVideoOptions() const { return static_cast<MJPEGOptions *>(video_options_.get()); ;}
	MJPEGOptions *GetVideoOptions() { return static_cast<MJPEGOptions *>(video_options_.get()); ;}
	MJPEGOptions *GetLoresOptions() const { return static_cast<MJPEGOptions *>(lores_options_.get()); ;}
	MJPEGOptions *GetLoresOptions() { return static_cast<MJPEGOptions *>(lores_options_.get()); ;}
	MJPEGOptions *GetImageOptions() const { return static_cast<MJPEGOptions *>(image_options_.get()); ;}
	MJPEGOptions *GetImageOptions() { return static_cast<MJPEGOptions *>(image_options_.get()); ;}

	void StopEncoders()
	{
		StopVideoEncoder();
		StopLoresEncoder();
		StopImageSaver();
	}
	void StopVideoEncoder() { 
		video_count++;
		SaveCount();
		video_encoder_.reset(); 
	}
	void StopLoresEncoder() { 
		lores_encoder_.reset(); 
	}
	void StopImageSaver() {
		SaveCount();
		image_saver_->stop();
		image_saver_.reset();
	}

	void InitialiseOptions() {
		MJPEGOptions *options = GetOptions();
		video_options_ = std::make_unique<MJPEGOptions>(*GetOptions());
		lores_options_ = std::make_unique<MJPEGOptions>(*GetOptions());
		image_options_ = std::make_unique<MJPEGOptions>(*GetOptions());
		LOG(2, "Initialising options...");

		lores_options_->output = options->output_preview;
		lores_options_->codec = "mjpeg";
		lores_options_->segment = 1;
		lores_options_->width = options->lores_width;
		lores_options_->height = options->lores_height;

		image_options_->quality = options->image_quality;
		image_options_->width = options->image_width;
		image_options_->height = options->image_height;
	}

protected:
	virtual void createVideoEncoder()
	{
		StreamInfo info;
		VideoStream(&info);
		if (!info.width || !info.height || !info.stride)
			throw std::runtime_error("video stream is not configured");

		// now we make changes to the video_options such that the encoder
		// outputs the file we want

		MJPEGOptions *video_options(GetVideoOptions());
		makeFilename(&(video_options->output), video_options->output_video);

		video_encoder_ = std::unique_ptr<Encoder>(Encoder::Create(video_options, info));
	}
	std::unique_ptr<Encoder> video_encoder_;

	virtual void createLoresEncoder()
	{
		StreamInfo info;
		LoresStream(&info);
		if (!info.width || !info.height || !info.stride)
			throw std::runtime_error("lores stream is not configured");
				
		MJPEGOptions *lores_options(GetLoresOptions());
		lores_encoder_ = std::unique_ptr<Encoder>(Encoder::Create(lores_options, info));
	}
	std::unique_ptr<Encoder> lores_encoder_;

	virtual void createImageSaver()
	{
		StreamInfo info;
		RawStream(&info);
		if (!info.width || !info.height || !info.stride)
			throw std::runtime_error("raw image stream is not configured");
		
		image_saver_ = std::unique_ptr<ImageSaver>(new ImageSaver(GetImageOptions(), CameraModel()));
	}
	std::unique_ptr<ImageSaver> image_saver_;

	std::unique_ptr<MJPEGOptions> video_options_;
	std::unique_ptr<MJPEGOptions> lores_options_;
	std::unique_ptr<MJPEGOptions> image_options_;


private:
	void videoEncodeBufferDone(void *mem)
	{
		// If non-NULL, mem would indicate which buffer has been completed, but
		// currently we're just assuming everything is done in order. (We could
		// handle this by replacing the queue with a vector of <mem, completed_request>
		// pairs.)
		assert(mem == nullptr);
		{
			std::lock_guard<std::mutex> lock(encode_buffer_queue_mutex_);
			if (encode_buffer_queue_.empty())
				throw std::runtime_error("no buffer available to return");
			CompletedRequestPtr &completed_request = encode_buffer_queue_.front();
			if (video_metadata_ready_callback_ && !GetOptions()->metadata.empty())
				video_metadata_ready_callback_(completed_request->metadata);
			encode_buffer_queue_.pop(); // drop shared_ptr reference
		}
	}
	void loresEncodeBufferDone(void *mem)
	{
		// If non-NULL, mem would indicate which buffer has been completed, but
		// currently we're just assuming everything is done in order. (We could
		// handle this by replacing the queue with a vector of <mem, completed_request>
		// pairs.)
		assert(mem == nullptr);
		{
			std::lock_guard<std::mutex> lock(encode_buffer_queue_mutex_);
			if (encode_buffer_queue_.empty())
				throw std::runtime_error("no buffer available to return");
			CompletedRequestPtr &completed_request = encode_buffer_queue_.front();
			if (lores_metadata_ready_callback_ && !GetOptions()->metadata.empty())
				lores_metadata_ready_callback_(completed_request->metadata);
			encode_buffer_queue_.pop(); // drop shared_ptr reference
		}
	}

	std::queue<CompletedRequestPtr> encode_buffer_queue_;
	std::mutex encode_buffer_queue_mutex_;
	EncodeOutputReadyCallback video_encode_output_ready_callback_;
	EncodeOutputReadyCallback lores_encode_output_ready_callback_;
	MetadataReadyCallback video_metadata_ready_callback_;
	MetadataReadyCallback lores_metadata_ready_callback_;

	struct timespec currTime;
	struct tm *localTime;

	int video_count = 0;
	int image_count = 0;


	// Attempt at reworking andrei's makeName function
	void makeName(std::string *name, std::string name_template) {
		
		size_t buffer_size = name_template.size() + MAX_UNSIGNED_INT_LENGTH + NULL_TERMINATOR_LENGTH;
		char* buffer = (char*)malloc(buffer_size);
		char* buffer_2 = (char*)malloc(buffer_size);

		std::time_t t = std::time(nullptr);
		std::tm* now = std::localtime(&t);

		const char *p = name_template.c_str();
		char *buf_p = buffer;

		while (*p)
		{
			if (*p == '%' && *(p + 1) == 'v') 
			{  // Custom specifier %v for video count
				strcpy (buf_p, std::to_string(video_count).c_str());
				buf_p += std::to_string(video_count).length();
				p += 2;  // Skip over %v
			}
			else if (*p == '%' && *(p + 1) == 'i') 
			{  // Custom specifier %i for image count
				strcpy (buf_p, std::to_string(image_count).c_str());
				buf_p += std::to_string(image_count).length();
				p += 2;  // Skip over %i
			}
			else
			{
				*buf_p++ = *p++;  // Copy regular characters
			}
		}
		*buf_p = '\0';  // Null-terminate the string

		std::strftime(buffer_2, buffer_size, buffer, now);
		*name = buffer_2;

		free(buffer);
		free(buffer_2);

		std::cout << "file name: " << *name << std::endl;
	}


	// makeFilename checks for the status of the provided path/filename and makes calls to makeName.
	void makeFilename(std::string* filename, std::string name_template) {
		char *name_template1;
		// if name_template is not an absolute path, prepend the media_path
		if (name_template[0] != '/') {
			asprintf(&name_template1,"%s/%s", GetOptions()->media_path.c_str(), name_template.c_str());
			makeName(filename, name_template1);
			// free(name_template1);
		} else {
			makeName(filename, name_template);
			// free(name_template1);
		}
	}

	void createPath(std::string path) {
		std::filesystem::path p(path);
		if (!std::filesystem::exists(p)) {
			std::filesystem::create_directories(p);
		}

		for (auto& path : fs::recursive_directory_iterator(p))
		{
			try {
				fs::permissions(path, fs::perms::all); // Uses fs::perm_options::replace.
			}
			catch (std::exception& e) {
				std::cerr << "Error changing permissions in media path: " << e.what() << std::endl;
			}           
		}
	}

	void createMediaPath() {
		createPath(GetOptions()->media_path);
	}
};
