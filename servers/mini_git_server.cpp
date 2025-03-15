#include "mini_git_server.h"

MiniGitServer::MiniGitServer(const std::filesystem::__cxx11::path& repoPath, int port)
        : repo(repoPath),
          acceptor(io_context, tcp::endpoint(tcp::v4(), port)) {

    startAccept();
}

void MiniGitServer::run() {
    std::cout << "MiniGit server started on port " << acceptor.local_endpoint().port() << std::endl;
    io_context.run();
}

void MiniGitServer::startAccept() {
    auto socket = std::make_shared<tcp::socket>(io_context);
    acceptor.async_accept(*socket, [this, socket](const boost::system::error_code& error) {
        if (!error) {
            std::thread(&MiniGitServer::handleClient, this, socket).detach();
        }

        startAccept();
    });
}

void MiniGitServer::handleClient(std::shared_ptr<tcp::socket> socket) {
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

        Response response;

        try {
            switch (requestType) {
                case RequestType::SAVE_FILE:
                {
                    std::string hash = repo.saveFile(request.fileName, request.content,
                                                     request.author, request.message, request.branch);
                    response.success = true;
                    response.message = "File saved with hash: " + hash;
                }
                    break;

                case RequestType::GET_LATEST:
                {
                    response.content = repo.getLatestVersion(request.fileName, request.branch);
                    response.success = true;
                    response.message = "Latest version retrieved";
                }
                    break;

                case RequestType::GET_VERSION:
                {
                    response.content = repo.getFileContent(request.fileName, request.version);
                    response.success = true;
                    response.message = "Version retrieved";
                }
                    break;

                case RequestType::GET_BRANCHES:
                {
                    response.branches = repo.getBranches();
                    response.success = true;
                    response.message = "Branches retrieved";
                }
                    break;

                case RequestType::GET_HISTORY:
                {
                    response.history = repo.getFileHistory(request.fileName);
                    response.success = true;
                    response.message = "History retrieved";
                }
                    break;
            }
        } catch (const std::exception& e) {
            response.success = false;
            response.message = e.what();
        }

        sendResponse(*socket, response);

    } catch (const std::exception& e) {
        std::cerr << "Error handling client: " << e.what() << std::endl;
    }

    socket->close();
}

void MiniGitServer::readString(tcp::socket& socket, std::string& str) {
    uint32_t length;
    boost::asio::read(socket, boost::asio::buffer(&length, sizeof(length)));

    str.resize(length);
    boost::asio::read(socket, boost::asio::buffer(str.data(), length));
}

void MiniGitServer::readBinaryData(tcp::socket& socket, std::vector<uint8_t>& data) {
    uint32_t length;
    boost::asio::read(socket, boost::asio::buffer(&length, sizeof(length)));

    data.resize(length);
    boost::asio::read(socket, boost::asio::buffer(data.data(), length));
}

void MiniGitServer::writeString(tcp::socket& socket, const std::string& str) {
    uint32_t length = static_cast<uint32_t>(str.size());
    boost::asio::write(socket, boost::asio::buffer(&length, sizeof(length)));
    boost::asio::write(socket, boost::asio::buffer(str.data(), str.size()));
}

void MiniGitServer::writeBinaryData(tcp::socket& socket, const std::vector<uint8_t>& data) {
    uint32_t length = static_cast<uint32_t>(data.size());
    boost::asio::write(socket, boost::asio::buffer(&length, sizeof(length)));
    boost::asio::write(socket, boost::asio::buffer(data.data(), data.size()));
}

void MiniGitServer::sendResponse(tcp::socket& socket, const Response& response) {
    uint8_t success = response.success ? 1 : 0;
    boost::asio::write(socket, boost::asio::buffer(&success, sizeof(success)));

    writeString(socket, response.message);

    if (response.success) {
        if (!response.content.empty()) {
            writeBinaryData(socket, response.content);
        } else if (!response.branches.empty()) {
            uint32_t count = static_cast<uint32_t>(response.branches.size());
            boost::asio::write(socket, boost::asio::buffer(&count, sizeof(count)));

            for (const auto& branch : response.branches) {
                writeString(socket, branch);
            }
        } else if (!response.history.empty()) {
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