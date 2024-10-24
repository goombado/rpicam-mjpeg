// pipe.cpp

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdio> // For std::remove
#include <string.h>
#include <unordered_map>
#include <algorithm> // For std::transform
#include <cctype>    // For std::toupper

#include "pipe.hpp"
#include "rpicam_mjpeg_encoder.hpp"
#include "logging.hpp"

enum Flag {
    IO, // image-output
    MO, // mjpeg-output
    VO, // video-output
    MP, // media-path
    IC, // image-count
    VC, // video-count
    BR, // brightness
    SH, // sharpness
    CO, // contrast
    SA, // saturation
    RO, // rotation
    SS, // shutter-speed
    BI, // bitrate
    RU, // halt/restart rpicam-mjpeg, ru 0/1
    CA, // start/stop video capture, optional timeout after t seconds, ca 0/1 [t]
    IM, // capture image, im 1
    TL, // Stop/start timelapse, tl 0/1 [t]
    TV, // N * 1/10 seconds between images in timelapse, tv [n]
    VI, // Set video split interval in seconds, vi [n]
    OTHER
};

std::unordered_map<std::string, Flag> flag_map = {
    {"IO", IO},
    {"MO", MO},
    {"VO", VO},
    {"MP", MP},
    {"IC", IC},
    {"VC", VC},
    {"BR", BR},
    {"SH", SH},
    {"CO", CO},
    {"SA", SA},
    {"RO", RO},
    {"SS", SS},
    {"BI", BI},
    {"RU", RU},
    {"CA", CA},
    {"IM", IM},
    {"TL", TL},
    {"TV", TV},
    {"VI", VI}
};

bool isFloat(const std::string& s) {
    try {
        std::size_t pos;
        std::stof(s, &pos);
        return pos == s.size();  // Ensures entire string was parsed
    } catch (std::invalid_argument& e) {
        return false;  // Not a number
    } catch (std::out_of_range& e) {
        return false;  // Number is out of range for a double
    }
}

bool isInteger(const std::string& s)
{
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

Pipe::Pipe(const std::string &pipeName)
    : pipeName(pipeName), pipeDescriptor(-1), isOpen(false), isForWriting(false) {}

Pipe::~Pipe() {
    std::cout << "Pipe destructor called." << std::endl;
    closePipe();
}

bool Pipe::createPipe() {
    // Create the named FIFO (with read and write permissions for everyone)
    if (access(pipeName.c_str(), F_OK) != -1) {
        // The pipe already exists
        return true;
    }
    if (mkfifo(pipeName.c_str(), 0666) == -1) {
        std::cerr << "Failed to create pipe: " << pipeName << std::endl;
        return false;
    }
    return true;
}

bool Pipe::openPipe(bool forWriting) {
    // Open the pipe for reading or writing
    int flags = O_NONBLOCK;
    if (forWriting)
        flags |= O_WRONLY;
    else
        flags |= O_RDONLY;

    pipeDescriptor = open(pipeName.c_str(), flags);
    if (pipeDescriptor == -1)
    {
        std::cerr << "Failed to open pipe: " << pipeName << std::endl;
        std::cerr << strerror(errno) << std::endl;
        return false;
    }
    isOpen = true;
    isForWriting = forWriting;
    if (!forWriting)
    {
        pollFd.fd = pipeDescriptor;
        pollFd.events = POLLIN;
    }
    return true;
}

bool Pipe::readData(std::string &data) {
    if (!isOpen || isForWriting)
    {
        std::cerr << "Pipe is not open for reading." << std::endl;
        data = "";
        return false;
    }

    int pollResult = poll(&pollFd, 1, 0);
    LOG(2, "Poll result: " << pollResult);
    if (pollResult == 0)
        return false;
    else if (pollResult == -1)
    {
        std::cerr << "Error polling pipe: " << strerror(errno) << std::endl;
        return false;
    }

    char buffer[1024];
    ssize_t bytesRead = read(pipeDescriptor, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0)
    {
        buffer[bytesRead] = '\0'; // Null-terminate the buffer
        LOG(2, "Read " << bytesRead << " bytes from pipe: " << buffer);
        data = std::string(buffer);
        return true;
    }

    return false; // Return an empty string if nothing was read
}

bool Pipe::writeData(const std::string& data) {
    if (!isOpen || !isForWriting)
    {
        std::cerr << "Pipe is not open for writing." << std::endl;
        return false;
    }

    ssize_t bytesWritten = write(pipeDescriptor, data.c_str(), data.size());
    if (bytesWritten == -1)
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            std::cerr << "Write failed: Pipe is full or no reader is present." << std::endl;
            return false;
        }
    return bytesWritten == static_cast<ssize_t>(data.size());
}

void Pipe::closePipe() {
    if (isOpen) {
        close(pipeDescriptor);
        isOpen = false;
    }
}

bool Pipe::removePipe() {
    // Remove the named pipe from the filesystem
    // return (unlink(pipeName.c_str()) == 0);
    std::cout << "Removing pipe: " << pipeName << std::endl;
    return (std::remove(pipeName.c_str()) == 0);
}

std::string to_upper(std::string input) {
    std::transform(input.begin(), input.end(), input.begin(),
                   ::toupper); // Use the global scope toupper
    return input;
}

void Pipe::readFIFO(RPiCamMJPEGEncoder *app) {

    // if (!openPipe(false)) {
    //     std::cerr << "Failed to open pipe for reading." << std::endl;
    //     return;
    // }

    std::string pipe_data;
    if (!readData(pipe_data))
        return;

    std::transform(pipe_data.begin(), pipe_data.end(), pipe_data.begin(), ::toupper);

    if (pipe_data[pipe_data.size() - 1] == '\n')
        pipe_data.pop_back();
    LOG(2, "Read data from pipe: " << pipe_data);
    app->SetLastFifoCommand(pipe_data);
    
    Flag flag;

    std::stringstream ss(pipe_data);
    std::string command_raw;
    ss >> command_raw;
    std::string command = to_upper(command_raw);
    LOG(2, "Command: " << command);

    if (flag_map.find(command) != flag_map.end())
    {
        flag = flag_map[command];
        std::cout << "Flag: " << flag << std::endl;
    }
    else 
        flag = OTHER;

    std::string arg;

    switch (flag)
    {
        case IO: // image-output
            app->WriteOptionToConfigFile(command_raw, arg);
            app->SetFifoRequest(FIFORequest::UNKNOWN);
            break;

        case MO: // mjpeg-output
            app->WriteOptionToConfigFile(command_raw, arg);
            app->SetFifoRequest(FIFORequest::UNKNOWN);
            break;

        case VO: // video-output
            app->WriteOptionToConfigFile(command_raw, arg);
            app->SetFifoRequest(FIFORequest::UNKNOWN);
            break;

        case MP: // media-path
            app->WriteOptionToConfigFile(command_raw, arg);
            app->SetFifoRequest(FIFORequest::UNKNOWN);
            break;

        case IC: // image-count
            app->WriteOptionToConfigFile(command_raw, arg);
            app->SetFifoRequest(FIFORequest::UNKNOWN);
            break;

        case VC: // video-count
            app->WriteOptionToConfigFile(command_raw, arg);
            app->SetFifoRequest(FIFORequest::UNKNOWN);
            break; 

        case BR: // brightness
            ss >> arg;
            if (!isFloat(arg))
                app->SetFifoRequest(FIFORequest::UNKNOWN);
            else
                app->WriteOptionToConfigFile("brightness", arg);
            break;
        
        case SH: // sharpness
            ss >> arg;
            if (!isFloat(arg))
                app->SetFifoRequest(FIFORequest::UNKNOWN);
            else
                app->WriteOptionToConfigFile("sharpness", arg);
            break;

        case CO: // contrast
            ss >> arg;
            if (!isFloat(arg))
                app->SetFifoRequest(FIFORequest::UNKNOWN);
            else
                app->WriteOptionToConfigFile("contrast", arg);
            break;
        
        case SA: // saturation
            ss >> arg;
            if (!isFloat(arg))
                app->SetFifoRequest(FIFORequest::UNKNOWN);
            else
                app->WriteOptionToConfigFile("saturation", arg);
            break;
        
        case RO: // rotation
            ss >> arg;
            if (!isInteger(arg))
                app->SetFifoRequest(FIFORequest::UNKNOWN);
            else
                app->WriteOptionToConfigFile("rotation", arg);
            break;
        
        case SS: // shutter-speed in microseconds
            ss >> arg;
            if (!isInteger(arg))
                app->SetFifoRequest(FIFORequest::UNKNOWN);
            else
                app->WriteOptionToConfigFile("shutter", arg);
            break;

        case BI: // bitrate in bits per second for h264 encoder
            ss >> arg;
            if (!isInteger(arg))
                app->SetFifoRequest(FIFORequest::UNKNOWN);
            else
                app->WriteOptionToConfigFile("bitrate", arg);
            break;

        case RU: // halt/restart rpicam-mjpeg, ru 0/1
            // check if next word is 0 or 1
            // if 0, stop rpicam-mjpeg
            // if 1, start rpicam-mjpeg
            if (ss.eof()) {
                app->SetFifoRequest(FIFORequest::UNKNOWN);
                std::cerr << "Must specify 0 or 1 with command \"ru\"." << std::endl;
            }
            else
            {
                ss >> arg;
                if (arg == "0")
                    app->SetFifoRequest(FIFORequest::STOP);
                else if (arg == "1")
                    app->SetFifoRequest(FIFORequest::RESTART);
                else
                    app->SetFifoRequest(FIFORequest::UNKNOWN);
            }
            break;
        
        case CA: // start/stop video capture, ca 0/1 [t]
            if (ss.eof())
                app->SetFifoRequest(FIFORequest::UNKNOWN);
            else
            {
                ss >> arg;
                if (arg == "0")
                    app->SetFifoRequest(FIFORequest::STOP_VIDEO);
                else if (arg == "1")
                {
                    if (app->IsVideoOutputting())
                        app->SetFifoRequest(FIFORequest::STOP_VIDEO);
                    else
                        app->SetFifoRequest(FIFORequest::START_VIDEO);
                    if (!ss.eof())
                    {
                        ss >> arg;
                        if (!isInteger(arg))
                            app->SetVideoCaptureDuration(std::stoi(arg));
                        else
                            app->SetFifoRequest(FIFORequest::UNKNOWN);
                    }
                }
                else
                    app->SetFifoRequest(FIFORequest::UNKNOWN);
            }
            break;
        
        case IM: // capture image, im
            app->SetFifoRequest(FIFORequest::CAPTURE_IMAGE);
            break;
        
        case TL: // Stop/start timelapse, tl 0/1 [t]
            app->SetFifoRequest(FIFORequest::UNKNOWN);
            break;
        
        case TV: // N * 1/10 seconds between images in timelapse, tv [n]
            app->SetFifoRequest(FIFORequest::UNKNOWN);
            break;
        
        case VI: // Set video split interval in seconds, vi [n]
            ss >> arg;
            if (!isInteger(arg))
                app->SetVideoSplitInterval(std::stoi(arg));
            else
                app->SetFifoRequest(FIFORequest::UNKNOWN);
            break;
            
        default:
            app->SetFifoRequest(FIFORequest::UNKNOWN);
            break;
    }
}