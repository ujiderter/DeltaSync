#ifndef DELTASYNC_FILE_VERSION_H
#define DELTASYNC_FILE_VERSION_H

#include <chrono>
#include <string>

struct FileVersion {
    std::string hash;
    std::string parentHash;
    std::chrono::system_clock::time_point timestamp;
    std::string author;
    std::string message;
    bool isDelta;
};
#endif //DELTASYNC_FILE_VERSION_H
