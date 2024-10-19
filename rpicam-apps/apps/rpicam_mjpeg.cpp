/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * rpicam_vid.cpp - libcamera video record app.
 */

#include <chrono>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>

#include "core/rpicam_mjpeg_encoder.hpp"
#include "output/output.hpp"

using namespace std::placeholders;

// Some keypress/signal handling.

static int signal_received;
static void default_signal_handler(int signal_number)
{
	signal_received = signal_number;
	LOG(2, "Received signal " << signal_number);
}

static int get_key_or_signal(MJPEGOptions const *options, pollfd p[1])
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

// The main event loop for the application.

enum class OutputState
{
	None0 = 0,
	Video0 = 1,
	None1 = 2,
	Image = 3,
	None2 = 4,
	Video1PrePhoto = 5,
	Video1PostPhoto = 6,
	None3 = 7
};

static std::unique_ptr<Output> createVideoOutput(MJPEGOptions const *options, RPiCamMJPEGEncoder &app)
{
	std::unique_ptr<Output> video_output =  std::unique_ptr<Output>(Output::Create((VideoOptions*) options));
	app.SetVideoEncodeOutputReadyCallback(std::bind(&Output::OutputReady, video_output.get(), _1, _2, _3, _4));
	app.SetVideoMetadataReadyCallback(std::bind(&Output::MetadataReady, video_output.get(), _1));
	return video_output;
}

static std::unique_ptr<Output> createLoresOutput(MJPEGOptions const *options, RPiCamMJPEGEncoder &app)
{
	std::unique_ptr<Output> lores_output = std::unique_ptr<Output>(Output::Create((VideoOptions*) options));
	app.SetLoresEncodeOutputReadyCallback(std::bind(&Output::OutputReady, lores_output.get(), _1, _2, _3, _4));
	app.SetLoresMetadataReadyCallback(std::bind(&Output::MetadataReady, lores_output.get(), _1));
	return lores_output;
}

static std::unique_ptr<Output> startVideoOutput(MJPEGOptions const *options, RPiCamMJPEGEncoder &app)
{
	std::unique_ptr<Output> video_output = createVideoOutput(options, app);
	app.StartVideoEncoder();
	return video_output;
}

static std::unique_ptr<Output> startLoresOutput(MJPEGOptions const *options, RPiCamMJPEGEncoder &app)
{
	std::unique_ptr<Output> lores_output = createLoresOutput(options, app);
	app.StartLoresEncoder();
	return lores_output;
}

static void stopVideoOutput(std::unique_ptr<Output> &video_output, RPiCamMJPEGEncoder &app)
{
	app.StopVideoEncoder();
	video_output.reset();
}

static void stopLoresOutput(std::unique_ptr<Output> &lores_output, RPiCamMJPEGEncoder &app)
{
	app.StopLoresEncoder();
	lores_output.reset();
}

static void teardownMJPEG(RPiCamMJPEGEncoder &app, std::unique_ptr<Output> &video_output, std::unique_ptr<Output> &lores_output)
{
	app.StopCamera();

	// stop active encoders to ensure buffers can be unnmapped in app.Teardown()
	if (app.IsVideoOutputting())
		stopVideoOutput(video_output, app);
	if (app.IsLoresOutputting())
		stopLoresOutput(lores_output, app);

	app.Teardown();
}

static void configureImage(RPiCamMJPEGEncoder &app)
{
	MJPEGOptions const *options = app.GetOptions();

	unsigned int still_flags = RPiCamApp::FLAG_STILL_NONE;
	if (options->encoding == "rgb24" || options->encoding == "png")
		still_flags |= RPiCamApp::FLAG_STILL_BGR;
	if (options->encoding == "rgb48")
		still_flags |= RPiCamApp::FLAG_STILL_BGR48;
	else if (options->encoding == "bmp")
		still_flags |= RPiCamApp::FLAG_STILL_RGB;
	if (options->raw)
		still_flags |= RPiCamApp::FLAG_STILL_RAW;

	app.ConfigureMJPEGImage(still_flags);
	std::cout << "MJPEG still configured" << std::endl;
	app.StartCamera();
	std::cout << "Camera started" << std::endl;
}

static void saveImage(CompletedRequestPtr &completed_request, RPiCamMJPEGEncoder &app)
{
	app.SaveImage(completed_request, app.ImageStream());
}

static std::unique_ptr<Output> startMJPEG(RPiCamMJPEGEncoder &app)
{
	app.ConfigureMJPEG();

	std::unique_ptr<Output> lores_output = startLoresOutput(app.GetLoresOptions(), app);
	if (!app.IsImageSaverStarted())
		app.StartImageSaver();
	app.StartCamera();

	return lores_output;
}

static std::unique_ptr<Output> restartMJPEG(RPiCamMJPEGEncoder &app, std::unique_ptr<Output> &video_output, std::unique_ptr<Output> &lores_output)
{
	teardownMJPEG(app, video_output, lores_output);
	return startMJPEG(app);
}

static void stopMJPEG(RPiCamMJPEGEncoder &app, std::unique_ptr<Output> &video_output, std::unique_ptr<Output> &lores_output)
{
	if (app.IsImageSaverStarted())
		app.StopImageSaver();

	teardownMJPEG(app, video_output, lores_output);
}

static void event_loop(RPiCamMJPEGEncoder &app)
{
	MJPEGOptions const *options = app.GetOptions();
	MJPEGOptions const *video_options = app.GetVideoOptions();

	std::unique_ptr<Output> video_output;
	std::unique_ptr<Output> lores_output;

	app.OpenCamera();
	lores_output = startMJPEG(app);

	auto start_time = std::chrono::high_resolution_clock::now();
	auto last_time = std::chrono::high_resolution_clock::now();
	OutputState state = OutputState::None1;

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
		RPiCamMJPEGEncoder::Msg msg = app.Wait();
		if (msg.type == RPiCamApp::MsgType::Timeout)
		{
			LOG_ERROR("ERROR: Device timeout detected, attempting a restart!!!");
			app.StopCamera();
			app.StartCamera();
			continue;
		}
		if (msg.type == RPiCamMJPEGEncoder::MsgType::Quit)
			return;
		else if (msg.type != RPiCamMJPEGEncoder::MsgType::RequestComplete)
			throw std::runtime_error("unrecognised message!");
		
		int key = get_key_or_signal(options, p);
		if (key == '\n' && app.IsVideoOutputting())
			video_output->Signal();

		LOG(2, "Viewfinder frame " << count);
		auto now = std::chrono::high_resolution_clock::now();
		bool timeout = !options->frames && options->timeout &&
					   ((now - start_time) > options->timeout.value);
		bool frameout = options->frames && count >= options->frames;
		if (timeout || frameout || key == 'x' || key == 'X')
		{
			if (timeout)
				LOG(2, "Halting: reached timeout of " << options->timeout.get<std::chrono::milliseconds>()
													  << " milliseconds.");
			stopMJPEG(app, video_output, lores_output);
			return;
		}
		CompletedRequestPtr &completed_request = std::get<CompletedRequestPtr>(msg.payload);

		if (state != OutputState::Image)
			app.LoresEncodeBuffer(completed_request, app.LoresStream());

		auto time_passed = now - last_time;
		switch (state)
		{
		case OutputState::None0:
			if (time_passed > std::chrono::milliseconds(5000))
			{
				std::cout << "Starting video output" << std::endl;
				state = OutputState::Video0;
				video_output = startVideoOutput(video_options, app);
				last_time = now;
			}
			break;
		case OutputState::Video0:
			app.VideoEncodeBuffer(completed_request, app.VideoStream());
			if (time_passed > std::chrono::milliseconds(5000))
			{
				std::cout << "Stopping video output" << std::endl;
				stopVideoOutput(video_output, app);
				state = OutputState::None1;
				last_time = now;
			}
			break;
		case OutputState::None1:
			if (time_passed > std::chrono::milliseconds(5000))
			{
				std::cout << "Began saving image at " << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() << std::endl;
				state = OutputState::Image;
				last_time = now;
				if (!options->image_no_teardown)
				{
					teardownMJPEG(app, video_output, lores_output);
					configureImage(app);
				}
			}
			break;
		case OutputState::Image:
			state = OutputState::None2;
			last_time = now;
			saveImage(completed_request, app);

			if (!options->image_no_teardown)
				lores_output = restartMJPEG(app, video_output, lores_output);

			break;
		case OutputState::None2:
			if (time_passed > std::chrono::milliseconds(5000))
			{
				std::cout << "Starting video output" << std::endl;
				state = OutputState::Video1PrePhoto;
				video_output = startVideoOutput(video_options, app);
				last_time = now;
			}
			break;
		case OutputState::Video1PrePhoto:
			app.VideoEncodeBuffer(completed_request, app.VideoStream());
			if (time_passed > std::chrono::milliseconds(5000))
			{
				std::cout << "Saving image" << std::endl;
				// saveImage(completed_request, app);
				state = OutputState::Video1PostPhoto;
				last_time = now;
			}
			break;
		case OutputState::Video1PostPhoto:
			app.VideoEncodeBuffer(completed_request, app.VideoStream());
			if (time_passed > std::chrono::milliseconds(5000))
			{
				std::cout << "Stopping video output" << std::endl;
				stopVideoOutput(video_output, app);
				state = OutputState::None3;
				last_time = now;
			}
			break;
		case OutputState::None3:
			if (time_passed > std::chrono::milliseconds(5000))
			{
				std::cout << "Shutting down application" << std::endl;
				app.StopCamera();
				app.StopEncoders();
				return;
			}
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	try
	{
		RPiCamMJPEGEncoder app;
		MJPEGOptions *options = app.GetOptions();
		if (options->Parse(argc, argv))
		{
			if (options->verbose >= 2)
				options->Print();
			
			app.InitialiseCount();
			app.InitialiseOptions();
			LOG(2, "past initialise options");
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
