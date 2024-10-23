/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * rpicam_vid.cpp - libcamera video record app.
 */

#include <chrono>
#include <thread>
#include <poll.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>

#include "core/rpicam_mjpeg_encoder.hpp"
#include "output/output.hpp"
#include "core/pipe.hpp"

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

enum class DemoState
{
	None0 = 0,
	Video0 = 1,
	None1 = 2,
	Image = 3,
	None2 = 4,
	Video1PrePhoto = 5,
	Video1Photo = 6,
	Video1PostPhoto = 7,
	None3 = 8
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
	std::unique_ptr<Output> lores_output = std::unique_ptr<Output>(Output::Create((VideoOptions*) options, &app));
	app.SetLoresEncodeOutputReadyCallback(std::bind(&Output::OutputReady, lores_output.get(), _1, _2, _3, _4));
	app.SetLoresMetadataReadyCallback(std::bind(&Output::MetadataReady, lores_output.get(), _1));
	return lores_output;
}

static std::unique_ptr<Output> startVideoOutput(MJPEGOptions const *options, RPiCamMJPEGEncoder &app, bool new_video = false)
{
	std::unique_ptr<Output> video_output = createVideoOutput(options, app);
	app.StartVideoEncoder();
	if (new_video)
		app.UpdateLastVideoCaptureTime();

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
	app.RequestImage();
}

static void saveImage(CompletedRequestPtr &completed_request, RPiCamMJPEGEncoder &app)
{
	app.SaveImage(completed_request, app.ImageStream());
}

static void prepareMJPEG(RPiCamMJPEGEncoder &app, bool restart = false)
{
	app.InitialiseCount();

	if (restart)
		app.ReinitialiseOptions();
	else
	{
		app.InitialiseOptions();
		app.InitialisePipes();
	}
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

// static std::unique_ptr<Output> restartMJPEG(RPiCamMJPEGEncoder &app, std::unique_ptr<Output> &video_output, std::unique_ptr<Output> &lores_output, bool reinit_options = true)
// {
// 	teardownMJPEG(app, video_output, lores_output);
// 	prepareMJPEG(app, reinit_options);
// 	return startMJPEG(app);
// }

static void stopMJPEG(RPiCamMJPEGEncoder &app, std::unique_ptr<Output> &video_output, std::unique_ptr<Output> &lores_output)
{
	if (app.IsImageSaverStarted())
		app.StopImageSaver();
	
	teardownMJPEG(app, video_output, lores_output);
}

static std::unique_ptr<Output> encodeVideoBuffer(CompletedRequestPtr &completed_request, RPiCamMJPEGEncoder &app, std::unique_ptr<Output> &video_output)
{
	app.VideoEncodeBuffer(completed_request, app.VideoStream());
	return std::move(video_output);
	// auto now = std::chrono::high_resolution_clock::now();
	// if (now - app.GetLastVideoCaptureTime() >= app.GetVideoCaptureDuration())
	// {
	// 	LOG(1, "Video timeout reached, stopping video encoder");
	// 	stopVideoOutput(video_output, app);
	// }
	// else if (now - app.GetLastVideoSplitTime() >= app.GetVideoSplitInterval())
	// {
	// 	LOG(2, "Video segment timeout reached, saving to new video file");
	// 	stopVideoOutput(video_output, app);
	// 	app.UpdateLastVideoSplitTime();
	// 	std::unique_ptr<Output> new_video_output = startVideoOutput(app.GetVideoOptions(), app);
	// 	return new_video_output;
	// }

	// return std::move(video_output);
}

static void event_loop_demo(RPiCamMJPEGEncoder &app)
{
	prepareMJPEG(app);

	MJPEGOptions const *options = app.GetOptions();
	MJPEGOptions const *video_options = app.GetVideoOptions();

	std::unique_ptr<Output> video_output;
	std::unique_ptr<Output> lores_output;

	app.OpenCamera();
	lores_output = startMJPEG(app);

	auto start_time = std::chrono::high_resolution_clock::now();
	auto last_time = std::chrono::high_resolution_clock::now();

	// this is relevant only for demo mode
	DemoState state = DemoState::None0;

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
			throw std::runtime_error("unrecognised message received from camera!");
		
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

		if (state != DemoState::Image && state != DemoState::Video1Photo)
			app.LoresEncodeBuffer(completed_request, app.LoresStream());

		auto time_passed = now - last_time;
		switch (state)
		{
		case DemoState::None0:
			if (time_passed <= std::chrono::milliseconds(10000))
				break;
	
			std::cout << "Starting video output" << std::endl;
			state = DemoState::Video0;
			video_output = startVideoOutput(video_options, app);
			last_time = now;
			break;
		case DemoState::Video0:
			app.VideoEncodeBuffer(completed_request, app.VideoStream());
			if (time_passed <= std::chrono::milliseconds(10000))
				break;

			std::cout << "Stopping video output" << std::endl;
			stopVideoOutput(video_output, app);
			state = DemoState::None1;
			last_time = now;
			break;
		case DemoState::None1:
			if (time_passed <= std::chrono::milliseconds(10000))
				break;
			std::cout << "Began saving image at " << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() << std::endl;
			state = DemoState::Image;
			last_time = now;
			if (!options->image_no_teardown)
			{
				teardownMJPEG(app, video_output, lores_output);
				configureImage(app);
			}
			break;
		case DemoState::Image:
			state = DemoState::None2;
			last_time = now;
			saveImage(completed_request, app);
			if (!options->image_no_teardown)
			{
				// teardownMJPEG(app, video_output, lores_output);
				app.StopCamera();
				app.Teardown();
				lores_output = startMJPEG(app);
			}
			break;
		case DemoState::None2:
			if (time_passed <= std::chrono::milliseconds(10000))
				break;
			std::cout << "Starting video output" << std::endl;
			state = DemoState::Video1PrePhoto;
			video_output = startVideoOutput(video_options, app);
			last_time = now;
			break;
		case DemoState::Video1PrePhoto:
			video_output = encodeVideoBuffer(completed_request, app, video_output);
			if (time_passed <= std::chrono::milliseconds(10000))
				break;

			if (!options->image_no_teardown)
			{
				std::cout << "Configuring image" << std::endl;
				teardownMJPEG(app, video_output, lores_output);
				configureImage(app);
			}
			state = DemoState::Video1Photo;
			last_time = now;
			break;
		case DemoState::Video1Photo:
			state = DemoState::Video1PostPhoto;
			last_time = now;
			saveImage(completed_request, app);
			if (!options->image_no_teardown)
			{
				app.StopCamera();
				app.Teardown();
				lores_output = startMJPEG(app);
				video_output = startVideoOutput(video_options, app);
			}
			break;
		case DemoState::Video1PostPhoto:
			video_output = encodeVideoBuffer(completed_request, app, video_output);
			if (time_passed <= std::chrono::milliseconds(10000))
				break;
			std::cout << "Stopping video output" << std::endl;
			stopVideoOutput(video_output, app);
			state = DemoState::None3;
			last_time = now;
			break;
		case DemoState::None3:
			if (time_passed <= std::chrono::milliseconds(10000))
				break;
			std::cout << "Shutting down application" << std::endl;
			app.StopCamera();
			app.StopEncoders();
			return;
		}
	}
}

static void event_loop(RPiCamMJPEGEncoder &app)
{
	prepareMJPEG(app);

	MJPEGOptions const *options = app.GetOptions();
	MJPEGOptions const *video_options = app.GetVideoOptions();

	std::unique_ptr<Output> video_output;
	std::unique_ptr<Output> lores_output;

	app.OpenCamera();
	lores_output = startMJPEG(app);

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
		// if number of microseconds since last fifo read time exceeds options->fifo_interval, read from fifo
		if (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - app.GetLastFifoReadTime()).count() >= options->fifo_interval)
			app.ReadControlFIFO();

		// now we check if we need to process any commands
		FIFORequest fifo_request = app.GetFifoRequest();
		if (fifo_request != FIFORequest::NONE)
		{	
			app.ResetFifoRequest();
			switch (fifo_request)
			{
				case FIFORequest::UNKNOWN:
					LOG(2, "Unknown command: " + app.GetLastFifoCommand());
					break;
				case FIFORequest::STOP:
					LOG(2, "Stopping application");
					// app.ClosePipes();
					stopMJPEG(app, video_output, lores_output);
					return;
				case FIFORequest::RESTART:
					LOG(2, "Restarting application");
					teardownMJPEG(app, video_output, lores_output);
					prepareMJPEG(app, false);
					lores_output = startMJPEG(app);
					break;
				case FIFORequest::START_VIDEO:
					video_output = startVideoOutput(video_options, app, true);
					break;
				case FIFORequest::STOP_VIDEO:
					stopVideoOutput(video_output, app);
					break;
				case FIFORequest::CAPTURE_IMAGE:
					if (!options->image_no_teardown)
					{
						teardownMJPEG(app, video_output, lores_output);
						configureImage(app);
					}
					app.RequestImage();
					break;
				case FIFORequest::TIMELAPSE:
					std::cout << "Timelapse not implemented" << std::endl;
					break;
				case FIFORequest::NONE:
					LOG(2, "No command received");
					break;
			}
		}
		
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
			throw std::runtime_error("unrecognised message received from camera!");
		
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
		
		if (app.IsImageRequested())
		{
			saveImage(completed_request, app);
			if (!options->image_no_teardown)
			{
				app.StopCamera();
				app.Teardown();
				lores_output = startMJPEG(app);
				continue;
			}
		}

		app.LoresEncodeBuffer(completed_request, app.LoresStream());
		if (app.IsVideoOutputting())
			video_output = encodeVideoBuffer(completed_request, app, video_output);
	}
}

int main(int argc, char *argv[])
{
	std::cout << "Starting rpicam_mjpeg" << std::endl;

	try
	{
		RPiCamMJPEGEncoder app;
		MJPEGOptions *options = app.GetOptions();
		if (options->Parse(argc, argv))
		{
			if (options->verbose >= 2)
				options->Print();

			if (options->demo)
				event_loop_demo(app);
			else
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
