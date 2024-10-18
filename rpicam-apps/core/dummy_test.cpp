// dummy_test.cpp

#include "pipe.hpp"
#include <iostream>
#include <sys/stat.h>
#include <thread>
#include <chrono>

// Function to test writing to the pipe
void testWrite(Pipe& pipe, const std::string& message) {
    if (!pipe.openPipe(true)) {
        std::cerr << "Failed to open pipe for writing." << std::endl;
        return;
    }
    if (pipe.writeData(message)) {
        std::cout << "Write successful: " << message << std::endl;
    } else {
        std::cerr << "Failed to write data to the pipe." << std::endl;
    }
    pipe.closePipe();
}

// Function to test reading from the pipe
void testRead(Pipe& pipe) {
    if (!pipe.openPipe(false)) {
        std::cerr << "Failed to open pipe for reading." << std::endl;
        return;
    }
    std::string data = pipe.readData();
    if (!data.empty()) {
        std::cout << "Read successful: " << data << std::endl;
    } else {
        std::cerr << "Failed to read data from the pipe or no data available." << std::endl;
    }
    pipe.closePipe();
}

bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);  // stat() returns 0 if the file exists
}


int main() {
    const std::string pipeName = "/var/www/FIFO";

    // Create a Pipe instance
    Pipe pipe(pipeName);
    
    // Create the named pipe (FIFO)
    if (!pipe.createPipe()) {
        std::cerr << "Failed to create the named pipe." << std::endl;
        return 1;
    }

    // Test writing to the pipe
    const std::string message = "Hello, this is a test message!";
    std::thread writerThread(testWrite, std::ref(pipe), message);

    // Give the writer some time to write before reading
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Test reading from the pipe
    std::thread readerThread(testRead, std::ref(pipe));

    // Wait for both threads to complete
    writerThread.join();
    readerThread.join();
    
    // std::this_thread::sleep_for(std::chrono::seconds(100));

    // Clean up: remove the named pipe
    if (!pipe.removePipe()) {
        std::cerr << "Failed to remove the named pipe." << std::endl;
    }

    // pipe.~Pipe();

    // if (!fileExists(pipeName)) {
    //     std::cout << "Confirmed: Pipe does not exist." << std::endl;
    // } else {
    //     std::cerr << "Error: Pipe still exists!" << std::endl;
    // }


    std::cout << "Exiting main." << std::endl;
    return 0;
}
