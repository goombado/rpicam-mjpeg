/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * rpicam_vid.cpp - libcamera video record app.
 */

#include <chrono>
#include <poll.h>
#include <signal.h>
#include <string>
#include <sys/signalfd.h>
#include <sys/stat.h>

#include "core/rpicam_raspimjpeg_encoder.hpp"
#include "output/output.hpp"
#include "core/mjpeg_options.hpp"

using namespace std::placeholders;

// Some keypress/signal handling.

static int signal_received;
static void default_signal_handler(int signal_number)
{
	signal_received = signal_number;
	LOG(1, "Received signal " << signal_number);
}

static int get_key_or_signal(VideoOptions const *options, pollfd p[1])
{
	int key = 0;
	if (signal_received == SIGINT)
		return 'x';
	if (options->keypress)
	{
		poll(p, 1, 0);
		if (p[0].revents & POLLIN)
		{
			char *user_string = nullptr;
			size_t len;
			[[maybe_unused]] size_t r = getline(&user_string, &len, stdin);
			key = user_string[0];
		}
	}
	if (options->signal)
	{
		if (signal_received == SIGUSR1)
			key = '\n';
		else if ((signal_received == SIGUSR2) || (signal_received == SIGPIPE))
			key = 'x';
		signal_received = 0;
	}
	return key;
}

// static int get_colourspace_flags(std::string const &codec)
// {
// 	if (codec == "mjpeg" || codec == "yuv420")
// 		return RPiCamRaspiMJPEGEncoder::FLAG_VIDEO_JPEG_COLOURSPACE;
// 	else
// 		return RPiCamRaspiMJPEGEncoder::FLAG_VIDEO_NONE;
// }

// The main even loop for the application.

static void event_loop(RPiCamRaspiMJPEGEncoder &app)
{
	VideoOptions *options_temp = app.GetOptions();
    options_temp->output = "test.mjpeg";
    options_temp->codec = "mjpeg";
    options_temp->quality = 50;
    const VideoOptions *options_mjpeg(options_temp);
	std::unique_ptr<Output> output_mjpeg = std::unique_ptr<Output>(Output::Create(options_mjpeg));
    
    options_temp->output = "test.mp4";
    options_temp->codec = "h264";
    const VideoOptions *options_video = options_temp;
    std::unique_ptr<Output> output_video = std::unique_ptr<Output>(Output::Create(options_video));
    
    options_temp->output = "preview.mp4";
    const VideoOptions *options_preview = options_temp;
    std::unique_ptr<Output> output_preview = std::unique_ptr<Output>(Output::Create(options_preview));

	app.SetEncodeOutputReadyCallbackMJPEG(std::bind(&Output::OutputReady, output_mjpeg.get(), _1, _2, _3, _4));
    app.SetEncodeOutputReadyCallbackVideo(std::bind(&Output::OutputReady, output_video.get(), _1, _2, _3, _4));
    app.SetEncodeOutputReadyCallbackPreview(std::bind(&Output::OutputReady, output_preview.get(), _1, _2, _3, _4));
	app.SetMetadataReadyCallback(std::bind(&Output::MetadataReady, output_video.get(), _1));

    VideoOptions const *options = app.GetOptions();


	app.OpenCamera();
    app.ConfigureRaspiMJPEG();
	app.StartEncoders();
	app.StartCamera();
	auto start_time = std::chrono::high_resolution_clock::now();

	// Monitoring for keypresses and signals.
	signal(SIGUSR1, default_signal_handler);
	signal(SIGUSR2, default_signal_handler);
	signal(SIGINT, default_signal_handler);
	// SIGPIPE gets raised when trying to write to an already closed socket. This can happen, when
	// you're using TCP to stream to VLC and the user presses the stop button in VLC. Catching the
	// signal to be able to react on it, otherwise the app terminates.
	signal(SIGPIPE, default_signal_handler);
	pollfd p[1] = { { STDIN_FILENO, POLLIN, 0 } };

	for (unsigned int count = 0; ; count++)
	{
		RPiCamRaspiMJPEGEncoder::Msg msg = app.Wait();
		if (msg.type == RPiCamApp::MsgType::Timeout)
		{
			LOG_ERROR("ERROR: Device timeout detected, attempting a restart!!!");
			app.StopCamera();
			app.StartCamera();
			continue;
		}
		if (msg.type == RPiCamRaspiMJPEGEncoder::MsgType::Quit)
			return;
		else if (msg.type != RPiCamRaspiMJPEGEncoder::MsgType::RequestComplete)
			throw std::runtime_error("unrecognised message!");
		int key = get_key_or_signal(options, p);
		if (key == '\n')
			output_mjpeg->Signal();
            output_video->Signal();
            output_preview->Signal();

		LOG(2, "Viewfinder frame " << count);
		auto now = std::chrono::high_resolution_clock::now();
		bool timeout = !options->frames && options->timeout &&
					   ((now - start_time) > options->timeout.value);
		bool frameout = options->frames && count >= options->frames;
		if (timeout || frameout || key == 'x' || key == 'X')
		{
			if (timeout)
				LOG(1, "Halting: reached timeout of " << options->timeout.get<std::chrono::milliseconds>()
													  << " milliseconds.");
			app.StopCamera(); // stop complains if encoder very slow to close
			app.StopEncoders();

			return;
		}

		CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);
		app.EncodeBufferMJPEG(completed_request, app.MJPEGStream());
        app.EncodeBufferVideo(completed_request, app.VideoStream());
        app.EncodeBufferPreview(completed_request, app.VideoStream());
	}
}

int main(int argc, char *argv[])
{

	try
	{
		RPiCamRaspiMJPEGEncoder app;
		MJPEGOptions mjpeg_options;

		if (mjpeg_options.Parse(argc, argv))
		{

			if (mjpeg_options.verbose >= 2)
				{
					std::cout << "here" << std::endl;
					mjpeg_options.Print();
				}
			event_loop(app);
		}
	}
	catch (std::exception const &e)
	{
		LOG_ERROR("ERROR: *** " << e.what() << " ***");
		return -1;
	}
	return 0;
}
