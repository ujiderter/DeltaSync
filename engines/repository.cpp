#include "repository.h"

Repository::Repository(const std::filesystem::__cxx11::path& path) : repoPath(path) {
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }

    std::filesystem::create_directories(path / "objects");
    std::filesystem::create_directories(path / "branches");

    if (!std::filesystem::exists(path / "branches" / "master")) {
        std::ofstream branch(path / "branches" / "master");
        branch.close();
    }

    loadRepository();
}

// Загрузка состояния репозитория с диска
void Repository::loadRepository() {
    std::lock_guard<std::mutex> lock(repoMutex);

    for (const auto& entry : std::filesystem::__cxx11::directory_iterator(repoPath / "branches")) {
        std::string branchName = entry.path().filename().string();
        branches[branchName] = {};
    }
}

// Сохранение файла в репозиторий
std::string Repository::saveFile(const std::string& fileName, const std::vector<uint8_t>& content,
                     const std::string& author, const std::string& message,
                     const std::string& branch = "master") {
    std::lock_guard<std::mutex> lock(repoMutex);

    std::string fileHash = DiffEngine::computeHash(content);

    bool isNewFile = fileVersions.find(fileName) == fileVersions.end() ||
                     fileVersions[fileName].empty();

    FileVersion newVersion;
    newVersion.hash = fileHash;
    newVersion.timestamp = std::chrono::system_clock::now();
    newVersion.author = author;
    newVersion.message = message;

    std::filesystem::__cxx11::path objectPath = repoPath / "objects" / fileHash;
    std::ofstream objectFile(objectPath, std::ios::binary);
    objectFile.write(reinterpret_cast<const char*>(content.data()), content.size());
    objectFile.close();

    if (!isNewFile) {
        std::string latestVersionHash = getCurrentVersionHash(fileName, branch);

        std::string newBranch = branch;
        if (latestVersionHash != fileVersions[fileName].back().hash) {
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::system_clock::to_time_t(now);
            std::stringstream branchName;
            branchName << branch << "-" << timestamp;
            newBranch = branchName.str();

            branches[newBranch] = branches[branch];
        }

        auto lastContent = getFileContent(fileName, latestVersionHash);

        auto delta = DiffEngine::computeDelta(lastContent, content);
        std::string deltaHash = DiffEngine::computeHash(delta);

        std::filesystem::__cxx11::path deltaPath = repoPath / "objects" / deltaHash;
        std::ofstream deltaFile(deltaPath, std::ios::binary);
        deltaFile.write(reinterpret_cast<const char*>(delta.data()), delta.size());
        deltaFile.close();

        newVersion.parentHash = latestVersionHash;
        newVersion.isDelta = true;
        newVersion.hash = deltaHash;

        branches[newBranch][fileName] = deltaHash;
    } else {
        newVersion.isDelta = false;
        newVersion.parentHash = "";

        branches[branch][fileName] = fileHash;
    }

    fileVersions[fileName].push_back(newVersion);

    return newVersion.hash;
}

// Получение содержимого файла по хешу
std::vector<uint8_t> Repository::getFileContent(const std::string& fileName, const std::string& hash) {
    std::lock_guard<std::mutex> lock(repoMutex);

    FileVersion* version = nullptr;
    for (const auto& v : fileVersions[fileName]) {
        if (v.hash == hash) {
            version = const_cast<FileVersion*>(&v);
            break;
        }
    }

    if (!version) {
        throw std::runtime_error("Version not found");
    }

    if (!version->isDelta) {
        std::filesystem::__cxx11::path objectPath = repoPath / "objects" / hash;
        std::ifstream file(objectPath, std::ios::binary);

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> content(size);
        file.read(reinterpret_cast<char*>(content.data()), size);

        return content;
    }

    auto parentContent = getFileContent(fileName, version->parentHash);

    std::filesystem::__cxx11::path deltaPath = repoPath / "objects" / hash;
    std::ifstream deltaFile(deltaPath, std::ios::binary);

    deltaFile.seekg(0, std::ios::end);
    size_t deltaSize = deltaFile.tellg();
    deltaFile.seekg(0, std::ios::beg);

    std::vector<uint8_t> delta(deltaSize);
    deltaFile.read(reinterpret_cast<char*>(delta.data()), deltaSize);

    return DiffEngine::applyDelta(parentContent, delta);
}

std::vector<uint8_t> Repository::getLatestVersion(const std::string& fileName, const std::string& branch = "master") {
    std::string hash = getCurrentVersionHash(fileName, branch);
    return getFileContent(fileName, hash);
}

std::string Repository::getCurrentVersionHash(const std::string& fileName, const std::string& branch = "master") {
    std::lock_guard<std::mutex> lock(repoMutex);

    if (branches.find(branch) == branches.end() ||
        branches[branch].find(fileName) == branches[branch].end()) {
        throw std::runtime_error("File not found in branch");
    }

    return branches[branch][fileName];
}

std::vector<std::string> Repository::getBranches() {
    std::lock_guard<std::mutex> lock(repoMutex);

    std::vector<std::string> result;
    for (const auto& [branch, _] : branches) {
        result.push_back(branch);
    }

    return result;
}

std::vector<FileVersion> Repository::getFileHistory(const std::string& fileName) {
    std::lock_guard<std::mutex> lock(repoMutex);

    if (fileVersions.find(fileName) == fileVersions.end()) {
        return {};
    }

    return fileVersions[fileName];
}

void Repository::deleteFile(const std::string& fileName, const std::string& branch) {
    std::lock_guard<std::mutex> lock(repoMutex);

    // Проверяем, существует ли файл в указанной ветке
    if (branches.find(branch) == branches.end() || branches[branch].find(fileName) == branches[branch].end()) {
        throw std::runtime_error("File or branch does not exist.");
    }

    // Создаем новую версию файла с отметкой "удален"
    FileVersion deletedVersion;
    deletedVersion.hash = computeHash({}); // Хеш пустого содержимого
    deletedVersion.parentHash = getCurrentVersionHash(fileName, branch);
    deletedVersion.timestamp = std::chrono::system_clock::now();
    deletedVersion.author = "System";
    deletedVersion.message = "File marked as deleted.";
    deletedVersion.isDelta = false;

    // Добавляем версию в историю файла
    fileVersions[fileName].push_back(deletedVersion);

    // Обновляем ветку, указывая на новую версию
    branches[branch][fileName] = deletedVersion.hash;

    std::cout << "File '" << fileName << "' marked as deleted in branch '" << branch << "'." << std::endl;
}

void Repository::deleteBranch(const std::string& branchName) {
    std::lock_guard<std::mutex> lock(repoMutex);

    // Проверяем, существует ли ветка
    if (branches.find(branchName) == branches.end()) {
        throw std::runtime_error("Branch does not exist.");
    }

    // Помечаем ветку как удаленную
    branches.erase(branchName);

    std::cout << "Branch '" << branchName << "' has been deleted." << std::endl;
}

void Repository::restoreFile(const std::string& fileName, const std::string& branch) {
    std::lock_guard<std::mutex> lock(repoMutex);

    // Проверяем, существует ли файл в указанной ветке
    if (branches.find(branch) == branches.end() || branches[branch].find(fileName) == branches[branch].end()) {
        throw std::runtime_error("File or branch does not exist.");
    }

    // Находим последнюю версию файла
    const auto& versions = fileVersions[fileName];
    if (versions.empty()) {
        throw std::runtime_error("No versions available for the file.");
    }

    const auto& lastVersion = versions.back();

    // Если файл уже не удален, ничего делать не нужно
    if (!lastVersion.isDeleted) {
        throw std::runtime_error("File is not marked as deleted.");
    }

    // Создаем новую версию файла с отметкой "восстановлен"
    FileVersion restoredVersion;
    restoredVersion.hash = lastVersion.parentHash; // Восстанавливаем предыдущую версию
    restoredVersion.parentHash = lastVersion.hash;
    restoredVersion.timestamp = std::chrono::system_clock::now();
    restoredVersion.author = "System";
    restoredVersion.message = "File restored.";
    restoredVersion.isDelta = false;
    restoredVersion.isDeleted = false;

    // Добавляем версию в историю файла
    fileVersions[fileName].push_back(restoredVersion);

    // Обновляем ветку, указывая на новую версию
    branches[branch][fileName] = restoredVersion.hash;

    std::cout << "File '" << fileName << "' has been restored in branch '" << branch << "'." << std::endl;
}