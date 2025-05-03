#include "mini_git_server.h"

namespace deltasync {

MiniGitServer::MiniGitServer(const std::filesystem::path& repoPath, int port)
    : repo(repoPath),
      acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {
    
    startAccept();
}

void MiniGitServer::run() {
    std::cout << "MiniGit server started on port " << acceptor.local_endpoint().port() << std::endl;
    running = true;
    io_context.run();
}

void MiniGitServer::stop() {
    running = false;
    io_context.stop();
    
    for (auto& thread : worker_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads.clear();
    
    std::cout << "MiniGit server stopped" << std::endl;
}

void MiniGitServer::startAccept() {
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context);
    
    acceptor.async_accept(*socket, [this, socket](const boost::system::error_code& error) {
        if (!error && running) {
            auto thread = std::thread(&MiniGitServer::handleClient, this, socket);
            
            worker_threads.push_back(std::move(thread));

            if (worker_threads.back().joinable()) {
                worker_threads.back().detach();
            }
        }

        if (running) {
            startAccept();
        }
    });
}

Response MiniGitServer::processRequest(const Request& request) {
    Response response;
    
    try {
        switch (request.type) {
            case RequestType::SAVE_FILE: {
                std::string hash = repo.saveFile(
                    request.fileName, 
                    request.content,
                    request.author, 
                    request.message, 
                    request.branch
                );
                response.success = true;
                response.message = "File saved with hash: " + hash;
                break;
            }
                
            case RequestType::GET_LATEST: {
                response.content = repo.getLatestVersion(request.fileName, request.branch);
                response.success = true;
                response.message = "Latest version retrieved";
                break;
            }
                
            case RequestType::GET_VERSION: {
                response.content = repo.getFileContent(request.fileName, request.version);
                response.success = true;
                response.message = "Version retrieved";
                break;
            }
                
            case RequestType::GET_BRANCHES: {
                response.branches = repo.getBranches();
                response.success = true;
                response.message = "Branches retrieved";
                break;
            }
                
            case RequestType::GET_HISTORY: {
                response.history = repo.getFileHistory(request.fileName);
                response.success = true;
                response.message = "History retrieved";
                break;
            }
        }
    } catch (const std::exception& e) {
        response.success = false;
        response.message = e.what();
    }
    
    return response;
}

void MiniGitServer::handleClient(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    try {
        uint32_t requestTypeInt;
        boost::asio::read(*socket, boost::asio::buffer(&requestTypeInt, sizeof(requestTypeInt)));
        RequestType requestType = static_cast<RequestType>(requestTypeInt);

        Request request;
        request.type = requestType;

        switch (requestType) {
            case RequestType::SAVE_FILE:
                readString(*socket, request.fileName);
                readString(*socket, request.branch);
                readString(*socket, request.author);
                readString(*socket, request.message);
                readBinaryData(*socket, request.content);
                break;

            case RequestType::GET_LATEST:
                readString(*socket, request.fileName);
                readString(*socket, request.branch);
                break;

            case RequestType::GET_VERSION:
                readString(*socket, request.fileName);
                readString(*socket, request.version);
                break;

            case RequestType::GET_BRANCHES:
                break;

            case RequestType::GET_HISTORY:
                readString(*socket, request.fileName);
                break;
        }

        Response response = processRequest(request);

        sendResponse(*socket, response);

    } catch (const std::exception& e) {
        std::cerr << "Error handling client: " << e.what() << std::endl;
        
        try {
            Response errorResponse;
            errorResponse.success = false;
            errorResponse.message = "Server error: " + std::string(e.what());
            sendResponse(*socket, errorResponse);
        } catch (...) {
        }
    }
    try {
        socket->close();
    } catch (...) {
    }
}

void MiniGitServer::readString(boost::asio::ip::tcp::socket& socket, std::string& str) {
    uint32_t length;
    boost::asio::read(socket, boost::asio::buffer(&length, sizeof(length)));

    str.resize(length);
    if (length > 0) {
        boost::asio::read(socket, boost::asio::buffer(str.data(), length));
    }
}

void MiniGitServer::readBinaryData(boost::asio::ip::tcp::socket& socket, std::vector<uint8_t>& data) {
    uint32_t length;
    boost::asio::read(socket, boost::asio::buffer(&length, sizeof(length)));

    data.resize(length);
    if (length > 0) {
        boost::asio::read(socket, boost::asio::buffer(data.data(), length));
    }
}

void MiniGitServer::writeString(boost::asio::ip::tcp::socket& socket, const std::string& str) {
    uint32_t length = static_cast<uint32_t>(str.size());
    boost::asio::write(socket, boost::asio::buffer(&length, sizeof(length)));
    
    if (length > 0) {
        boost::asio::write(socket, boost::asio::buffer(str.data(), str.size()));
    }
}

void MiniGitServer::writeBinaryData(boost::asio::ip::tcp::socket& socket, const std::vector<uint8_t>& data) {
    uint32_t length = static_cast<uint32_t>(data.size());
    boost::asio::write(socket, boost::asio::buffer(&length, sizeof(length)));
    
    if (length > 0) {
        boost::asio::write(socket, boost::asio::buffer(data.data(), data.size()));
    }
}

void MiniGitServer::sendResponse(boost::asio::ip::tcp::socket& socket, const Response& response) {
    uint8_t success = response.success ? 1 : 0;
    boost::asio::write(socket, boost::asio::buffer(&success, sizeof(success)));

    writeString(socket, response.message);

    if (response.success) {
        if (!response.content.empty()) {
            writeBinaryData(socket, response.content);
        } 
        else if (!response.branches.empty()) {
            uint32_t count = static_cast<uint32_t>(response.branches.size());
            boost::asio::write(socket, boost::asio::buffer(&count, sizeof(count)));

            for (const auto& branch : response.branches) {
                writeString(socket, branch);
            }
        } 
        else if (!response.history.empty()) {
            uint32_t count = static_cast<uint32_t>(response.history.size());
            boost::asio::write(socket, boost::asio::buffer(&count, sizeof(count)));

            for (const auto& version : response.history) {
                writeString(socket, version.hash);
                writeString(socket, version.parentHash);

                auto timestamp = std::chrono::system_clock::to_time_t(version.timestamp);
                boost::asio::write(socket, boost::asio::buffer(&timestamp, sizeof(timestamp)));

                writeString(socket, version.author);
                writeString(socket, version.message);

                uint8_t isDelta = version.isDelta ? 1 : 0;
                boost::asio::write(socket, boost::asio::buffer(&isDelta, sizeof(isDelta)));
            }
        }
    }
}

} // namespace deltasync