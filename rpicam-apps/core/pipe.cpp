// pipe.cpp

#include "pipe.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdio> // For std::remove
#include <string.h>
#include <unordered_map>

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
    UNKNOWN
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
    {"BI", BI}
};

Pipe::Pipe(const std::string &pipeName)
    : pipeName(pipeName), pipeDescriptor(-1), isOpen(false), isForWriting(false) {}

Pipe::~Pipe() {
    std::cout << "Pipe destructor called." << std::endl;
    closePipe();
}

bool Pipe::createPipe() {
    // Create the named FIFO (with read and write permissions for everyone)
    if (mkfifo(pipeName.c_str(), 0666) == -1) {
        std::cerr << "Failed to create pipe: " << pipeName << std::endl;
        return false;
    }
    return true;
}

bool Pipe::openPipe(bool forWriting) {
    // Open the pipe for reading or writing
    pipeDescriptor = open(pipeName.c_str(), forWriting ? O_WRONLY : O_RDONLY);
    if (pipeDescriptor == -1) {
        std::cerr << "Failed to open pipe: " << pipeName << std::endl;
        return false;
    }
    isOpen = true;
    isForWriting = forWriting;
    return true;
}

std::string Pipe::readData() {
    if (!isOpen || isForWriting) {
        std::cerr << "Pipe is not open for reading." << std::endl;
        return "";
    }

    char buffer[1024];
    ssize_t bytesRead = read(pipeDescriptor, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0'; // Null-terminate the buffer
        return std::string(buffer);
    }

    return ""; // Return an empty string if nothing was read
}

bool Pipe::writeData(const std::string& data) {
    if (!isOpen || !isForWriting) {
        std::cerr << "Pipe is not open for writing." << std::endl;
        return false;
    }

    ssize_t bytesWritten = write(pipeDescriptor, data.c_str(), data.size());
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
    return (unlink(pipeName.c_str()) == 0);
}

static void readFIFO(const std::string &pipeName, RPiCamMJPEGEncoder *app) {
    Pipe pipe(pipeName);

    if (!pipe.openPipe(false)) {
        std::cerr << "Failed to open pipe for reading." << std::endl;
        return;
    }

    std::string pipe_data = pipe.readData();
    Flag flag;

    std::stringstream ss(pipe_data);
    std::string command;
    ss >> command;

    if (flag_map.find(command) != flag_map.end())
        flag = flag_map[command];
    else 
        flag = UNKNOWN;

    switch (flag)
    {
    case IO: // image-output
        break;

    case MO: // mjpeg-output
        break;

    case VO: // video-output
        break;

    case MP: // media-path
        break;

    case IC: // image-count
        
        break;
    
    case VC: // video-count
        break; 

    case BR: // brightness
        break;
    
    case SH: // sharpness
        break;

    case CO: // contrast
        break;
    
    case SA: // saturation
        break;
    
    case RO: // rotation
        break;
    
    case SS: // shutter-speed
        break;

    case BI: // bitrate
        break;
        
    default:
        break;
    }
}