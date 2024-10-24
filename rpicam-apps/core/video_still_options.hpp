/**
 * Combination of video_options.cpp and still_options.cpp
 * CHANGES:
 *  - quality refers to MJPEG quality and new image_quality refers to JPEG quality
 */


#pragma once

#include <cstdio>

#include <string>

#include "options.hpp"
#include "video_options.hpp"
#include "still_options.hpp"


struct VideoStillOptions : public Options
{
	VideoStillOptions() : Options()
	{
		using namespace boost::program_options;
		// first we setup VideoOptions::VideoOptions()

		// Generally we shall use zero or empty values to avoid over-writing the
		// codec's default behaviour.
		// clang-format off
		options_.add_options()
			("bitrate,b", value<std::string>(&bitrate_)->default_value("0bps"),
			 "Set the video bitrate for encoding. If no units are provided, default to bits/second.")
			("profile", value<std::string>(&profile),
			 "Set the encoding profile")
			("level", value<std::string>(&level),
			 "Set the encoding level")
			("intra,g", value<unsigned int>(&intra)->default_value(0),
			 "Set the intra frame period")
			("inline", value<bool>(&inline_headers)->default_value(false)->implicit_value(true),
			 "Force PPS/SPS header with every I frame (h264 only)")
			("codec", value<std::string>(&codec)->default_value("h264"),
			 "Set the codec to use, either h264, "
#if LIBAV_PRESENT
			  "libav, "
#endif
			  "mjpeg or yuv420")
			("save-pts", value<std::string>(&save_pts),
			 "Save a timestamp file with this name")
			("quality,q", value<int>(&quality)->default_value(50),
			 "Set the MJPEG quality parameter (mjpeg only)")
			("listen,l", value<bool>(&listen)->default_value(false)->implicit_value(true),
			 "Listen for an incoming client network connection before sending data to the client")
			("keypress,k", value<bool>(&keypress)->default_value(false)->implicit_value(true),
			 "Pause or resume video recording when ENTER pressed")
			("signal,s", value<bool>(&signal)->default_value(false)->implicit_value(true),
			 "Pause or resume video recording when signal received")
			("initial,i", value<std::string>(&initial)->default_value("record"),
			 "Use 'pause' to pause the recording at startup, otherwise 'record' (the default)")
			("split", value<bool>(&split)->default_value(false)->implicit_value(true),
			 "Create a new output file every time recording is paused and then resumed")
			("segment", value<uint32_t>(&segment)->default_value(0),
			 "Break the recording into files of approximately this many milliseconds")
			("circular", value<size_t>(&circular)->default_value(0)->implicit_value(4),
			 "Write output to a circular buffer of the given size (in MB) which is saved on exit")
			("frames", value<unsigned int>(&frames)->default_value(0),
			 "Run for the exact number of frames specified. This will override any timeout set.")
#if LIBAV_PRESENT
			("libav-video-codec", value<std::string>(&libav_video_codec)->default_value("h264_v4l2m2m"),
			 "Sets the libav video codec to use. "
			 "To list available codecs, run  the \"ffmpeg -codecs\" command.")
			("libav-video-codec-opts", value<std::string>(&libav_video_codec_opts),
			 "Sets the libav video codec options to use. "
			 "These override the internal defaults (check 'encoderOptions*()' in 'encoder/libav_encoder.cpp' for the defaults). "
			 "Separate key and value with \"=\" and multiple options with \";\". "
			 "e.g.: \"preset=ultrafast;profile=high;partitions=i8x8,i4x4\". "
			 "To list available options for a given codec, run the \"ffmpeg -h encoder=libx264\" command for libx264.")
			("libav-format", value<std::string>(&libav_format),
			 "Sets the libav encoder output format to use. "
			 "Leave blank to try and deduce this from the filename.\n"
			 "To list available formats, run  the \"ffmpeg -formats\" command.")
			("libav-audio", value<bool>(&libav_audio)->default_value(false)->implicit_value(true),
			 "Records an audio stream together with the video.")
			("audio-codec", value<std::string>(&audio_codec)->default_value("aac"),
			 "Sets the libav audio codec to use.\n"
			 "To list available codecs, run  the \"ffmpeg -codecs\" command.")
			("audio-source", value<std::string>(&audio_source)->default_value("pulse"),
			 "Audio source to record from. Valid options are \"pulse\" and \"alsa\"")
			("audio-device", value<std::string>(&audio_device)->default_value("default"),
			 "Audio device to record from.  To list the available devices,\n"
			 "for pulseaudio, use the following command:\n"
			 "\"pactl list | grep -A2 'Source #' | grep 'Name: '\"\n"
			 "or for alsa, use the following command:\n"
			 "\"arecord -L\"")
			("audio-channels", value<uint32_t>(&audio_channels)->default_value(0),
			 "Number of channels to use for recording audio. Set to 0 to use default value.")
			("audio-bitrate", value<std::string>(&audio_bitrate_)->default_value("32kbps"),
			 "Set the audio bitrate for encoding. If no units are provided, default to bits/second.")
			("audio-samplerate", value<uint32_t>(&audio_samplerate)->default_value(0),
			 "Set the audio sampling rate in Hz for encoding. Set to 0 to use the input sample rate.")
			("av-sync", value<std::string>(&av_sync_)->default_value("0us"),
			 "Add a time offset (in microseconds if no units provided) to the audio stream, relative to the video stream. "
			 "The offset value can be either positive or negative.")
#endif
		// clang-format on

		// next setup StillOptions::StillOptions()
			("image-quality,iq", value<int>(&image_quality)->default_value(93),
			 "Set the JPEG quality parameter")
			("exif,x", value<std::vector<std::string>>(&exif),
			 "Add these extra EXIF tags to the output file")
			("timelapse", value<std::string>(&timelapse_)->default_value("0ms"),
			 "Time interval between timelapse captures. If no units are provided default to ms.")
			("framestart", value<uint32_t>(&framestart)->default_value(0),
			 "Initial frame counter value for timelapse captures")
			("datetime", value<bool>(&datetime)->default_value(false)->implicit_value(true),
			 "Use date format for output file names")
			("timestamp", value<bool>(&timestamp)->default_value(false)->implicit_value(true),
			 "Use system timestamps for output file names")
			("restart", value<unsigned int>(&restart)->default_value(0),
			 "Set JPEG restart interval")
			("keypress,k", value<bool>(&keypress)->default_value(false)->implicit_value(true),
			 "Perform capture when ENTER pressed")
			("signal,s", value<bool>(&signal)->default_value(false)->implicit_value(true),
			 "Perform capture when signal received")
			("thumb", value<std::string>(&thumb)->default_value("320:240:70"),
			 "Set thumbnail parameters as width:height:quality, or none")
			("encoding,e", value<std::string>(&encoding)->default_value("jpg"),
			 "Set the desired output encoding, either jpg, png, rgb/rgb24, rgb48, bmp or yuv420")
			("raw,r", value<bool>(&raw)->default_value(false)->implicit_value(true),
			 "Also save raw file in DNG format")
			("latest", value<std::string>(&latest),
			 "Create a symbolic link with this name to most recent saved file")
			("autofocus-on-capture", value<bool>(&af_on_capture)->default_value(false)->implicit_value(true),
			 "Switch to AfModeAuto and trigger a scan just before capturing a still")
			("zsl", value<bool>(&zsl)->default_value(false)->implicit_value(true),
			 "Switch to AfModeAuto and trigger a scan just before capturing a still")
		;
	}


	// VideoOptions attributes
	Bitrate bitrate;
	std::string profile;
	std::string level;
	unsigned int intra;
	bool inline_headers;
	std::string codec;
	std::string libav_video_codec;
	std::string libav_video_codec_opts;
	std::string libav_format;
	bool libav_audio;
	std::string audio_codec;
	std::string audio_device;
	std::string audio_source;
	uint32_t audio_channels;
	Bitrate audio_bitrate;
	uint32_t audio_samplerate;
	TimeVal<std::chrono::microseconds> av_sync;
	std::string save_pts;
	int quality;
	bool listen;
	bool keypress;
	bool signal;
	std::string initial;
	bool pause;
	bool split;
	uint32_t segment;
	size_t circular;
	uint32_t frames;

	// StillOptions attributes
	int image_quality;
	std::vector<std::string> exif;
	TimeVal<std::chrono::milliseconds> timelapse;
	uint32_t framestart;
	bool datetime;
	bool timestamp;
	unsigned int restart;
	std::string thumb;
	unsigned int thumb_width, thumb_height, thumb_quality;
	std::string encoding;
	bool raw;
	std::string latest;
	bool zsl;

	std::string bitrate_;
#if LIBAV_PRESENT
	std::string av_sync_;
	std::string audio_bitrate_;
#endif /* LIBAV_PRESENT */
	std::string timelapse_;

	virtual bool Parse(int argc, char *argv[]) override
	{
		// VideoOptions::Parse()

		if (Options::Parse(argc, argv) == false)
			return false;

		bitrate.set(bitrate_);
#if LIBAV_PRESENT
		av_sync.set(av_sync_);
		audio_bitrate.set(audio_bitrate_);
#endif /* LIBAV_PRESENT */
		if (width == 0)
			width = 640;
		if (height == 0)
			height = 480;
		if (strcasecmp(codec.c_str(), "h264") == 0)
			codec = "h264";
		else if (strcasecmp(codec.c_str(), "libav") == 0)
			codec = "libav";
		else if (strcasecmp(codec.c_str(), "yuv420") == 0)
			codec = "yuv420";
		else if (strcasecmp(codec.c_str(), "mjpeg") == 0)
			codec = "mjpeg";
		else
			throw std::runtime_error("unrecognised codec " + codec);
		if (strcasecmp(initial.c_str(), "pause") == 0)
			pause = true;
		else if (strcasecmp(initial.c_str(), "record") == 0)
			pause = false;
		else
			throw std::runtime_error("incorrect initial value " + initial);
		if ((pause || split || segment || circular) && !inline_headers)
			LOG_ERROR("WARNING: consider inline headers with 'pause'/split/segment/circular");
		if ((split || segment) && output.find('%') == std::string::npos)
			LOG_ERROR("WARNING: expected % directive in output filename");

		// From https://en.wikipedia.org/wiki/Advanced_Video_Coding#Levels
		double mbps = ((width + 15) >> 4) * ((height + 15) >> 4) * framerate.value_or(DEFAULT_FRAMERATE);
		if ((codec == "h264" || (codec == "libav" && libav_video_codec == "libx264")) && mbps > 245760.0)
		{
			LOG(1, "Overriding H.264 level 4.2");
			level = "4.2";
		}


		// StillOptions::Parse()

		timelapse.set(timelapse_);

		if ((keypress || signal) && timelapse)
			throw std::runtime_error("keypress/signal and timelapse options are mutually exclusive");
		if (strcasecmp(thumb.c_str(), "none") == 0)
			thumb_quality = 0;
		else if (sscanf(thumb.c_str(), "%u:%u:%u", &thumb_width, &thumb_height, &thumb_quality) != 3)
			throw std::runtime_error("bad thumbnail parameters " + thumb);
		if (strcasecmp(encoding.c_str(), "jpg") == 0)
			encoding = "jpg";
		else if (strcasecmp(encoding.c_str(), "yuv420") == 0)
			encoding = "yuv420";
		else if (strcasecmp(encoding.c_str(), "rgb") == 0 || strcasecmp(encoding.c_str(), "rgb24") == 0)
			encoding = "rgb24";
		else if (strcasecmp(encoding.c_str(), "rgb48") == 0)
			encoding = "rgb48";
		else if (strcasecmp(encoding.c_str(), "png") == 0)
			encoding = "png";
		else if (strcasecmp(encoding.c_str(), "bmp") == 0)
			encoding = "bmp";
		else
			throw std::runtime_error("invalid encoding format " + encoding);
		return true;
	}

	virtual void ReconstructArgs(std::vector<std::string> &args) const override
	{
		Options::ReconstructArgs(args);

		if (!bitrate_.empty())
			args.push_back("--bitrate=" + bitrate_);
		if (!profile.empty())
			args.push_back("--profile=" + profile);
		if (!level.empty())
			args.push_back("--level=" + level);
		if (intra != 0)
			args.push_back("--intra=" + std::to_string(intra));
		if (inline_headers)
			args.push_back("--inline");
		if (!codec.empty())
			args.push_back("--codec=" + codec);
		if (!save_pts.empty())
			args.push_back("--save-pts=" + save_pts);
		if (quality != 50)
			args.push_back("--quality=" + std::to_string(quality));
		if (listen)
			args.push_back("--listen");
		if (keypress)
			args.push_back("--keypress");
		if (signal)
			args.push_back("--signal");
		if (!initial.empty())
			args.push_back("--initial=" + initial);
		if (split)
			args.push_back("--split");
		if (segment != 0)
			args.push_back("--segment=" + std::to_string(segment));
		if (circular != 0)
			args.push_back("--circular=" + std::to_string(circular));
		if (frames != 0)
			args.push_back("--frames=" + std::to_string(frames));
#if LIBAV_PRESENT
		if (!libav_video_codec.empty())
			args.push_back("--libav-video-codec=" + libav_video_codec);
		if (!libav_video_codec_opts.empty())
			args.push_back("--libav-video-codec-opts=" + libav_video_codec_opts);
		if (!libav_format.empty())
			args.push_back("--libav-format=" + libav_format);
		if (libav_audio)
			args.push_back("--libav-audio");
		if (!audio_codec.empty())
			args.push_back("--audio-codec=" + audio_codec);
		if (!audio_source.empty())
			args.push_back("--audio-source=" + audio_source);
		if (!audio_device.empty())
			args.push_back("--audio-device=" + audio_device);
		if (audio_channels != 0)
			args.push_back("--audio-channels=" + std::to_string(audio_channels));
		if (!audio_bitrate_.empty())
			args.push_back("--audio-bitrate=" + audio_bitrate_);
		if (audio_samplerate != 0)
			args.push_back("--audio-samplerate=" + std::to_string(audio_samplerate));
		if (!av_sync_.empty())
			args.push_back("--av-sync=" + av_sync_);
#endif
		if (image_quality != 93)
			args.push_back("--image-quality=" + std::to_string(image_quality));
		if (exif.size() > 0)
		{
			std::string e = "--exif=";
			for (const auto &tag : exif)
				e += tag + ",";
		}
		if (!timelapse_.empty())
			args.push_back("--timelapse=" + timelapse_);
		if (framestart != 0)
			args.push_back("--framestart=" + std::to_string(framestart));
		if (datetime)
			args.push_back("--datetime");
		if (timestamp)
			args.push_back("--timestamp");
		if (restart != 0)
			args.push_back("--restart=" + std::to_string(restart));
		if (!thumb.empty())
			args.push_back("--thumb=" + thumb);
		if (!encoding.empty())
			args.push_back("--encoding=" + encoding);
		if (raw)
			args.push_back("--raw");
		if (!latest.empty())
			args.push_back("--latest=" + latest);
		if (af_on_capture)
			args.push_back("--autofocus-on-capture");
		if (zsl)
			args.push_back("--zsl");

		return;
	}


	virtual void Print() const override
	{
		Options::Print();
		// VideoOptions::Print()
		std::cerr << "    bitrate: " << bitrate.kbps() << "kbps" << std::endl;
		std::cerr << "    profile: " << profile << std::endl;
		std::cerr << "    level:  " << level << std::endl;
		std::cerr << "    intra: " << intra << std::endl;
		std::cerr << "    inline: " << inline_headers << std::endl;
		std::cerr << "    save-pts: " << save_pts << std::endl;
		std::cerr << "    codec: " << codec << std::endl;
		std::cerr << "    quality (for MJPEG): " << quality << std::endl;
		std::cerr << "    keypress: " << keypress << std::endl;
		std::cerr << "    signal: " << signal << std::endl;
		std::cerr << "    initial: " << initial << std::endl;
		std::cerr << "    split: " << split << std::endl;
		std::cerr << "    segment: " << segment << std::endl;
		std::cerr << "    circular: " << circular << std::endl;

		// StillOptions::Print()
		std::cerr << "    encoding: " << encoding << std::endl;
		std::cerr << "    image_quality: " << image_quality << std::endl;
		std::cerr << "    raw: " << raw << std::endl;
		std::cerr << "    restart: " << restart << std::endl;
		std::cerr << "    timelapse: " << timelapse.get() << "ms" << std::endl;
		std::cerr << "    framestart: " << framestart << std::endl;
		std::cerr << "    datetime: " << datetime << std::endl;
		std::cerr << "    timestamp: " << timestamp << std::endl;
		std::cerr << "    keypress: " << keypress << std::endl;
		std::cerr << "    signal: " << signal << std::endl;
		std::cerr << "    thumbnail width: " << thumb_width << std::endl;
		std::cerr << "    thumbnail height: " << thumb_height << std::endl;
		std::cerr << "    thumbnail quality: " << thumb_quality << std::endl;
		std::cerr << "    latest: " << latest << std::endl;
		std::cerr << "    AF on capture: " << af_on_capture << std::endl;
		std::cerr << "    Zero shutter lag: " << zsl << std::endl;
		for (auto &s : exif)
			std::cerr << "    EXIF: " << s << std::endl;
	}

};
