#ifndef DELTASYNC_FILE_VERSION_H
#define DELTASYNC_FILE_VERSION_H

#include <chrono>
#include <string>
#include <ostream>

namespace deltasync {

struct FileVersion {
    std::string hash;          
    std::string parentHash;    
    std::chrono::system_clock::time_point timestamp; 
    std::string author;        
    std::string message;       
    bool isDelta;              


    FileVersion() = default;

    FileVersion(
        std::string hash,
        std::string parentHash,
        std::chrono::system_clock::time_point timestamp,
        std::string author,
        std::string message,
        bool isDelta
    ) : hash(std::move(hash)),
        parentHash(std::move(parentHash)),
        timestamp(timestamp),
        author(std::move(author)),
        message(std::move(message)),
        isDelta(isDelta) {}


    friend std::ostream& operator<<(std::ostream& os, const FileVersion& version) {
        auto time_t = std::chrono::system_clock::to_time_t(version.timestamp);
        os << "FileVersion{"
           << "hash: " << version.hash
           << ", parent: " << version.parentHash
           << ", time: " << std::ctime(&time_t)
           << ", author: " << version.author
           << ", message: " << version.message
           << ", isDelta: " << (version.isDelta ? "true" : "false")
           << "}";
        return os;
    }
};

} // namespace deltasync

#endif // DELTASYNC_FILE_VERSION_H