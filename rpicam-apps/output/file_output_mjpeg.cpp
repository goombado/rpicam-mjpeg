/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * file_output.cpp - Write output to file.
 */

#include "file_output_mjpeg.hpp"

FileOutputMJPEG::FileOutputMJPEG(VideoOptions const *options, RPiCamMJPEGEncoder *encoder)
    : Output(options), fp_(nullptr), file_start_time_ms_(0), count_(0), encoder_(encoder)
{
}

FileOutputMJPEG::~FileOutputMJPEG()
{
	closeFile();
}

void FileOutputMJPEG::outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t flags)
{
	// We need to open a new file if we're in "segment" mode and our segment is full
	// (though we have to wait for the next I frame), or if we're in "split" mode
	// and recording is being restarted (this is necessarily an I-frame already).
	if (fp_ == nullptr ||
		(options_->segment && (flags & FLAG_KEYFRAME) &&
		 timestamp_us / 1000 - file_start_time_ms_ > options_->segment) ||
		(options_->split && (flags & FLAG_RESTART)))
	{
		closeFile();
		openFile(timestamp_us);
	}

	LOG(2, "FileOutputMJPEG: output buffer " << mem << " size " << size);
	if (fp_ && size)
	{
		if (fwrite(mem, size, 1, fp_) != 1)
			throw std::runtime_error("failed to write output bytes");
		if (options_->flush)
			fflush(fp_);
	}
}

void FileOutputMJPEG::openFile(int64_t timestamp_us)
{
	if (options_->output == "-")
		fp_ = stdout;
	else if (!options_->output.empty())
	{
		LOG(2, "file output name not empty");
		// Generate the next output file name.
		char filename[256];
		int n = snprintf(filename, sizeof(filename), options_->output.c_str(), count_);
		count_++;
		if (options_->wrap)
			count_ = count_ % options_->wrap;
		if (n < 0)
			throw std::runtime_error("failed to generate filename");

		fp_ = fopen(filename, "w");
		if (!fp_)
			throw std::runtime_error("failed to open output file " + std::string(filename));
		LOG(2, "FileOutputMJPEG: opened output file " << filename);

		file_start_time_ms_ = timestamp_us / 1000;
	}
}

void FileOutputMJPEG::closeFile()
{
	if (fp_)
	{
		if (options_->flush)
			fflush(fp_);
		if (fp_ != stdout) {
			fclose(fp_);
		}
		fp_ = nullptr;
	}
	
    encoder_->MoveTempMJPEGOutput();
}
