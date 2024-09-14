/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * rpicam_encoder.cpp - libcamera video encoding class.
 */

#pragma once

#include "core/rpicam_app.hpp"
#include "core/stream_info.hpp"
#include "core/video_options.hpp"

#include "encoder/encoder.hpp"

typedef std::function<void(void *, size_t, int64_t, bool)> EncodeOutputReadyCallback;
typedef std::function<void(libcamera::ControlList &)> MetadataReadyCallback;

class RPiCamRaspiMJPEGEncoder : public RPiCamApp
{
public:
	using Stream = libcamera::Stream;
	using FrameBuffer = libcamera::FrameBuffer;

	RPiCamRaspiMJPEGEncoder() : RPiCamApp(std::make_unique<VideoOptions>()) {}

	void StartEncoders()
	{
		StartEncoderMJPEG();
		StartEncoderVideo();
		StartEncoderPreview();
		VideoOptions* options = GetOptions();
		options->output = "test.mjpeg"; // a hack for now, i hate pointers
	}

	void StartEncoderMJPEG()
	{
		createEncoderMJPEG();
		encoder_mjpeg_->SetInputDoneCallback(std::bind(&RPiCamRaspiMJPEGEncoder::encodeBufferDoneMJPEG, this, std::placeholders::_1));
		encoder_mjpeg_->SetOutputReadyCallback(encode_output_ready_callback_mjpeg_);
	}
	void StartEncoderVideo()
	{
		createEncoderVideo();
		encoder_video_->SetInputDoneCallback(std::bind(&RPiCamRaspiMJPEGEncoder::encodeBufferDoneVideo, this, std::placeholders::_1));
		encoder_video_->SetOutputReadyCallback(encode_output_ready_callback_video_);
	}
	void StartEncoderPreview()
	{
		createEncoderPreview();
		encoder_preview_->SetInputDoneCallback(std::bind(&RPiCamRaspiMJPEGEncoder::encodeBufferDonePreview, this, std::placeholders::_1));
		encoder_preview_->SetOutputReadyCallback(encode_output_ready_callback_preview_);
	}

	// This is callback when the encoder gives you the encoded output data.
	void SetEncodeOutputReadyCallbackMJPEG(EncodeOutputReadyCallback callback) { encode_output_ready_callback_mjpeg_ = callback; }
	void SetEncodeOutputReadyCallbackVideo(EncodeOutputReadyCallback callback) { encode_output_ready_callback_video_ = callback; }
	void SetEncodeOutputReadyCallbackPreview(EncodeOutputReadyCallback callback) { encode_output_ready_callback_preview_ = callback; }
	void SetMetadataReadyCallback(MetadataReadyCallback callback) { metadata_ready_callback_ = callback; }
	
	void EncodeBufferMJPEG(CompletedRequestPtr &completed_request, Stream *stream)
	{
		assert(encoder_mjpeg_);
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
			std::lock_guard<std::mutex> lock(encode_buffer_queue_mjpeg_mutex_);
			encode_buffer_queue_mjpeg_.push(completed_request); // creates a new reference
		}
		encoder_mjpeg_->EncodeBuffer(buffer->planes()[0].fd.get(), span.size(), mem, info, timestamp_ns / 1000);
	}

	void EncodeBufferVideo(CompletedRequestPtr &completed_request, Stream *stream)
	{
		assert(encoder_video_);
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
			std::lock_guard<std::mutex> lock(encode_buffer_queue_video_mutex_);
			encode_buffer_queue_video_.push(completed_request); // creates a new reference
		}
		encoder_video_->EncodeBuffer(buffer->planes()[0].fd.get(), span.size(), mem, info, timestamp_ns / 1000);
	}

	void EncodeBufferPreview(CompletedRequestPtr &completed_request, Stream *stream)
	{
		assert(encoder_preview_);
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
			std::lock_guard<std::mutex> lock(encode_buffer_queue_preview_mutex_);
			encode_buffer_queue_preview_.push(completed_request); // creates a new reference
		}
		encoder_preview_->EncodeBuffer(buffer->planes()[0].fd.get(), span.size(), mem, info, timestamp_ns / 1000);
	}


	VideoOptions *GetOptions() const { return static_cast<VideoOptions *>(options_.get()); }

	void StopEncoders() {
		StopEncoderMJPEG();
		StopEncoderVideo();
		StopEncoderPreview();
	}
	void StopEncoderMJPEG() { encoder_mjpeg_.reset(); }
	void StopEncoderVideo() { encoder_video_.reset(); }
	void StopEncoderPreview() { encoder_preview_.reset(); }

protected:
	virtual void createEncoderMJPEG()
	{
		StreamInfo info;
		MJPEGStream(&info);
		if (!info.width || !info.height || !info.stride)
			throw std::runtime_error("mjpeg stream is not configured");
		VideoOptions *options = GetOptions();
		options->output = "test.mjpeg";
		options->codec = "mjpeg";
		options->quality = 50;
		VideoOptions *mjpeg_options(options);
		encoder_mjpeg_ = std::unique_ptr<Encoder>(Encoder::Create(mjpeg_options, info));
	}

	virtual void createEncoderVideo()
	{
		StreamInfo info;
		VideoStream(&info);
		if (!info.width || !info.height || !info.stride)
			throw std::runtime_error("video stream is not configured");
		VideoOptions *options = GetOptions();
		options->output = "test.mp4";
		options->codec = "h264";
		encoder_video_ = std::unique_ptr<Encoder>(Encoder::Create(options, info));
	}

	virtual void createEncoderPreview()
	{
		StreamInfo info;
		VideoStream(&info);
		if (!info.width || !info.height || !info.stride)
			throw std::runtime_error("lores stream is not configured");
		VideoOptions *options = GetOptions();
		options->output = "lores.mp4";
		encoder_preview_ = std::unique_ptr<Encoder>(Encoder::Create(options, info));
	}

	std::unique_ptr<Encoder> encoder_mjpeg_;
	std::unique_ptr<Encoder> encoder_video_;
	std::unique_ptr<Encoder> encoder_preview_;

private:
	void encodeBufferDoneMJPEG(void *mem)
	{
		// If non-NULL, mem would indicate which buffer has been completed, but
		// currently we're just assuming everything is done in order. (We could
		// handle this by replacing the queue with a vector of <mem, completed_request>
		// pairs.)
		assert(mem == nullptr);
		{
			std::lock_guard<std::mutex> lock(encode_buffer_queue_mjpeg_mutex_);
			if (encode_buffer_queue_mjpeg_.empty())
				throw std::runtime_error("no buffer available to return");
			CompletedRequestPtr &completed_request = encode_buffer_queue_mjpeg_.front();
			if (metadata_ready_callback_ && !GetOptions()->metadata.empty())
				metadata_ready_callback_(completed_request->metadata);
			encode_buffer_queue_mjpeg_.pop(); // drop shared_ptr reference
		}
	}

	void encodeBufferDoneVideo(void *mem)
	{
		assert(mem == nullptr);
		{
			std::lock_guard<std::mutex> lock(encode_buffer_queue_video_mutex_);
			if (encode_buffer_queue_video_.empty())
				throw std::runtime_error("no buffer available to return");
			CompletedRequestPtr &completed_request = encode_buffer_queue_video_.front();
			if (metadata_ready_callback_ && !GetOptions()->metadata.empty())
				metadata_ready_callback_(completed_request->metadata);
			encode_buffer_queue_video_.pop(); // drop shared_ptr reference
		}
	}

	void encodeBufferDonePreview(void *mem)
	{
		assert(mem == nullptr);
		{
			std::lock_guard<std::mutex> lock(encode_buffer_queue_preview_mutex_);
			if (encode_buffer_queue_preview_.empty())
				throw std::runtime_error("no buffer available to return");
			CompletedRequestPtr &completed_request = encode_buffer_queue_preview_.front();
			if (metadata_ready_callback_ && !GetOptions()->metadata.empty())
				metadata_ready_callback_(completed_request->metadata);
			encode_buffer_queue_preview_.pop(); // drop shared_ptr reference
		}
	}

	std::queue<CompletedRequestPtr> encode_buffer_queue_mjpeg_;
	std::queue<CompletedRequestPtr> encode_buffer_queue_video_;
	std::queue<CompletedRequestPtr> encode_buffer_queue_preview_;
	std::mutex encode_buffer_queue_mjpeg_mutex_;
	std::mutex encode_buffer_queue_video_mutex_;
	std::mutex encode_buffer_queue_preview_mutex_;
	EncodeOutputReadyCallback encode_output_ready_callback_mjpeg_;
	EncodeOutputReadyCallback encode_output_ready_callback_video_;
	EncodeOutputReadyCallback encode_output_ready_callback_preview_;
	MetadataReadyCallback metadata_ready_callback_;
};
