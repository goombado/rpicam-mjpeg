// pipe.hpp

#pragma once

#include <string>
#include <poll.h>

class RPiCamMJPEGEncoder;

class Pipe {
public:
    // Constructor to initialize the pipe with a name
    explicit Pipe(const std::string& pipeName);
    
    // Destructor to clean up resources
    ~Pipe();

    // Method to create the named FIFO pipe
    bool createPipe();

    // Method to open the pipe for reading or writing
    bool openPipe(bool forWriting);

    // Method to read data from the pipe
    bool readData(std::string &data);

    // Method to close the pipe
    void closePipe();

    // Method to remove the named FIFO pipe from the filesystem
    bool removePipe();

    void readFIFO(RPiCamMJPEGEncoder *app);

private:
    std::string pipeName;
    int pipeDescriptor; // File descriptor for the pipe
    bool isOpen;        // Indicates if the pipe is currently open
    bool isForWriting;  // Indicates if the pipe is opened for writing
    struct pollfd pollFd;
};
