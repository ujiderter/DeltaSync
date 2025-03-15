#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <filesystem>
#include <algorithm>
#include <optional>
#include <chrono>
#include <mutex>
#include <thread>
#include <cstring>
#include <cstdint>
#include <boost/asio.hpp>
#include "engines/diff_engine.h"
#include "servers/file_version.h"
#include "engines/repository.h"
#include "servers/mini_git_server.h"

int main(int argc, char* argv[]) {
    try {
        int port = 8080;

        std::string repoPath = "./minigit_repo";

        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "--port" && i + 1 < argc) {
                port = std::stoi(argv[++i]);
            } else if (arg == "--repo" && i + 1 < argc) {
                repoPath = argv[++i];
            }
        }

        std::cout << "Starting MiniGit server on port " << port << std::endl;
        std::cout << "Repository path: " << repoPath << std::endl;

        MiniGitServer server(repoPath, port);
        server.run();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}