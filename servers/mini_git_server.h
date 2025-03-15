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

namespace fs = std::filesystem;
using boost::asio::ip::tcp;

// Класс для обработки сетевых запросов
class MiniGitServer {
private:
    Repository repo;
    boost::asio::io_context io_context;
    tcp::acceptor acceptor;

    // Структура сетевого запроса
    enum class RequestType {
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
        bool success;
        std::string message;
        std::vector<uint8_t> content;
        std::vector<std::string> branches;
        std::vector<FileVersion> history;
    };

public:
    MiniGitServer(const std::filesystem::__cxx11::path& repoPath, int port);

    void run();

private:
    void startAccept();
    void handleClient(std::shared_ptr<tcp::socket> socket);

    // Вспомогательные функции для чтения/записи данных по сети
    void readString(tcp::socket& socket, std::string& str);

    void readBinaryData(tcp::socket& socket, std::vector<uint8_t>& data);

    void writeString(tcp::socket& socket, const std::string& str);

    void writeBinaryData(tcp::socket& socket, const std::vector<uint8_t>& data);

    void sendResponse(tcp::socket& socket, const Response& response);
};

#endif //DELTASYNC_MINI_GIT_SERVER_H
