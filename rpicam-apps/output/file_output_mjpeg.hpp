/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * file_output.hpp - Write output to file.
 */

#pragma once

#include "output.hpp"

class FileOutputMJPEG : public Output
{
public:
	FileOutputMJPEG(VideoOptions const *options, RPiCamMJPEGEncoder *encoder);
	~FileOutputMJPEG();

protected:
	void outputBuffer(void *mem, size_t size, int64_t timestamp_us, uint32_t flags) override;

private:
	void closeFile();
	void openFile(int64_t timestamp_us);
	FILE *fp_;
	int64_t file_start_time_ms_;
	unsigned int count_;
    RPiCamMJPEGEncoder *encoder_;

};
