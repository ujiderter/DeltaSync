#ifndef DELTASYNC_MINI_GIT_SERVER_H
#define DELTASYNC_MINI_GIT_SERVER_H

#include "../engines/repository.h"
#include "file_version.h"
#include "../engines/diff_engine.h"

#include <boost/asio.hpp>
#include <cstdint>
#include <cstring>
#include <thread>
#include <mutex>
#include <chrono>
#include <optional>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <iostream>

namespace deltasync {

class MiniGitServer {
public:
    MiniGitServer(const std::filesystem::path& repoPath, int port);

    void run();

    void stop();

private:
    enum class RequestType : uint32_t {
        SAVE_FILE,     
        GET_LATEST,     
        GET_VERSION,    
        GET_BRANCHES,   
        GET_HISTORY    
    };

    struct Request {
        RequestType type;
        std::string fileName;
        std::string branch;
        std::string version;
        std::string author;
        std::string message;
        std::vector<uint8_t> content;
    };

    struct Response {
        bool success = false;
        std::string message;
        std::vector<uint8_t> content;
        std::vector<std::string> branches;
        std::vector<FileVersion> history;
    };

    Repository repo;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor;
    std::atomic<bool> running{true};
    std::vector<std::thread> worker_threads;

    void startAccept();

    void handleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    Response processRequest(const Request& request);

    void readString(boost::asio::ip::tcp::socket& socket, std::string& str);

    void readBinaryData(boost::asio::ip::tcp::socket& socket, std::vector<uint8_t>& data);

    void writeString(boost::asio::ip::tcp::socket& socket, const std::string& str);

    void writeBinaryData(boost::asio::ip::tcp::socket& socket, const std::vector<uint8_t>& data);

    void sendResponse(boost::asio::ip::tcp::socket& socket, const Response& response);
};

} // namespace deltasync

#endif // DELTASYNC_MINI_GIT_SERVER_H