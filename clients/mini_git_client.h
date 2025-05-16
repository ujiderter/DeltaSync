#ifndef DELTASYNC_MINI_GIT_CLIENT_H
#define DELTASYNC_MINI_GIT_CLIENT_H

#include <boost/asio.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>

namespace deltasync {

class MiniGitClient {
public:
    MiniGitClient(const std::string& serverIp, int port);
    
    bool saveFile(
        const std::string& fileName,
        const std::string& branch,
        const std::string& author,
        const std::string& message,
        const std::vector<uint8_t>& content
    );
    
    std::vector<uint8_t> getLatest(const std::string& fileName, const std::string& branch);
    std::vector<std::string> getBranches();
    std::vector<std::string> getHistory(const std::string& fileName, const std::string& branch);

private:
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket;
    
    void connect();
    void sendRequest(const std::vector<uint8_t>& requestData);
    std::vector<uint8_t> receiveResponse();
};

} // namespace deltasync

#endif // DELTASYNC_MINI_GIT_CLIENT_H