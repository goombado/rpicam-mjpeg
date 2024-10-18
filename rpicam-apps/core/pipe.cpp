// pipe.cpp

#include "pipe.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdio> // For std::remove

Pipe::Pipe(const std::string& pipeName)
    : pipeName(pipeName), pipeDescriptor(-1), isOpen(false), isForWriting(false) {}

Pipe::~Pipe() {
    closePipe();
    removePipe(); // Ensure the pipe is removed when the object is destroyed
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

void Pipe::removePipe() {
    // Remove the named pipe from the filesystem
    if (std::remove(pipeName.c_str()) != 0) {
        std::cerr << "Failed to remove pipe: " << pipeName << std::endl;
    }
}
