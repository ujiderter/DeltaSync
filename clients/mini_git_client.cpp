#include "mini_git_client.h"
#include <cstring>

namespace deltasync {

MiniGitClient::MiniGitClient(const std::string& serverIp, int port) 
    : socket(io_context) {
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::make_address(serverIp), 
        port
    );
    socket.connect(endpoint);
}

bool MiniGitClient::saveFile(
    const std::string& fileName,
    const std::string& branch,
    const std::string& author,
    const std::string& message,
    const std::vector<uint8_t>& content
) {
    std::vector<uint8_t> request;
    uint32_t requestType = 0;
    
    request.insert(request.end(), (uint8_t*)&requestType, (uint8_t*)&requestType + sizeof(uint32_t));
    
    auto pushStr = [&](const std::string& str) {
        uint32_t len = str.size();
        request.insert(request.end(), (uint8_t*)&len, (uint8_t*)&len + sizeof(uint32_t));
        request.insert(request.end(), str.begin(), str.end());
    };
    
    pushStr(fileName);
    pushStr(branch);
    pushStr(author);
    pushStr(message);
    request.insert(request.end(), content.begin(), content.end());
    
    sendRequest(request);
    auto response = receiveResponse();
    
    return response.size() > 0 && response[0] == 1; // success = true
}

std::vector<uint8_t> MiniGitClient::getLatest(const std::string& fileName, const std::string& branch) {
    uint32_t requestType = 1; // GET_LATEST
    std::vector<uint8_t> request;
    
    request.insert(request.end(), (uint8_t*)&requestType, (uint8_t*)&requestType + sizeof(uint32_t));
    
    auto pushStr = [&](const std::string& str) {
        uint32_t len = str.size();
        request.insert(request.end(), (uint8_t*)&len, (uint8_t*)&len + sizeof(uint32_t));
        request.insert(request.end(), str.begin(), str.end());
    };
    
    pushStr(fileName);
    pushStr(branch);
    
    sendRequest(request);
    return receiveResponse();
}

void MiniGitClient::sendRequest(const std::vector<uint8_t>& requestData) {
    boost::asio::write(socket, boost::asio::buffer(requestData));
}

std::vector<uint8_t> MiniGitClient::receiveResponse() {
    std::vector<uint8_t> buffer(1024);
    boost::system::error_code error;
    size_t len = socket.read_some(boost::asio::buffer(buffer), error);
    
    if (error) {
        throw boost::system::system_error(error);
    }
    
    buffer.resize(len);
    return buffer;
}

} // namespace deltasync