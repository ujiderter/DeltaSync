#ifndef DELTASYNC_REPOSITORY_H
#define DELTASYNC_REPOSITORY_H

#include "../servers/file_version.h"
#include "diff_engine.h"
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

class Repository {
private:
    std::filesystem::__cxx11::path repoPath;
    std::map<std::string, std::vector<FileVersion>> fileVersions;  // файл -> версии
    std::map<std::string, std::map<std::string, std::string>> branches;  // ветка -> (файл -> хеш)
    std::mutex repoMutex;

public:
    Repository(const std::filesystem::__cxx11::path& path);

    // Загрузка состояния репозитория с диска
    void loadRepository();

    // Сохранение файла в репозиторий
    std::string saveFile(const std::string& fileName, const std::vector<uint8_t>& content,
                        const std::string& author, const std::string& message,
                        const std::string& branch);

    // Получение содержимого файла по хешу
    std::vector<uint8_t> getFileContent(const std::string& fileName, const std::string& hash);

    // Получение последней версии файла в указанной ветке
    std::vector<uint8_t> getLatestVersion(const std::string& fileName, const std::string& branch );

    // Получение хеша текущей версии файла в указанной ветке
    std::string getCurrentVersionHash(const std::string& fileName, const std::string& branch);

    // Получение списка всех веток
    std::vector<std::string> getBranches();

    // Получение истории версий файла
    std::vector<FileVersion> getFileHistory(const std::string& fileName);
};

#endif //DELTASYNC_REPOSITORY_H
