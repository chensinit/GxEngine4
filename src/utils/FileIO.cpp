#include "FileIO.h"
#include "logger.h"

#if SDL2_ENGINE_PLATFORM_ANDROID
#include <android/asset_manager.h>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#else
#include <filesystem>
#endif

#include <fstream>
#include <sstream>

#if SDL2_ENGINE_PLATFORM_ANDROID
static AAssetManager* s_assetManager = nullptr;
static std::string s_dataPath;
#endif

std::string FileIO::readFileAsText(const std::string& path) {
#if SDL2_ENGINE_PLATFORM_ANDROID
    if (!s_assetManager) {
        Log::error("[FileIO] Android: setAssetManager() not called");
        return "";
    }
    AAsset* asset = AAssetManager_open(s_assetManager, path.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        Log::error("[FileIO] Android: failed to open asset: ", path);
        return "";
    }
    size_t length = static_cast<size_t>(AAsset_getLength(asset));
    std::string content;
    content.resize(length);
    int readLen = AAsset_read(asset, &content[0], static_cast<size_t>(length));
    AAsset_close(asset);
    if (readLen < 0 || static_cast<size_t>(readLen) != length) {
        Log::error("[FileIO] Android: failed to read asset: ", path);
        return "";
    }
    return content;
#else
    std::ifstream file(path);
    if (!file.is_open()) {
        Log::error("[FileIO] Failed to open: ", path);
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
#endif
}

bool FileIO::writeFileAsText(const std::string& path, const std::string& content) {
#if SDL2_ENGINE_PLATFORM_ANDROID
    std::string fullPath = s_dataPath.empty() ? path : (s_dataPath + "/" + path);
    std::ofstream file(fullPath);
    if (!file.is_open()) {
        Log::error("[FileIO] Android: failed to open for write: ", fullPath);
        return false;
    }
    file << content;
    return file.good();
#else
    std::ofstream file(path);
    if (!file.is_open()) {
        Log::error("[FileIO] Failed to open for write: ", path);
        return false;
    }
    file << content;
    return file.good();
#endif
}

bool FileIO::createDirectoriesForFile(const std::string& path) {
#if SDL2_ENGINE_PLATFORM_ANDROID
    if (s_dataPath.empty()) {
        return true;
    }
    std::string base = s_dataPath;
    size_t start = 0;
    while (start < path.size()) {
        size_t end = path.find('/', start);
        if (end == std::string::npos) break;
        std::string segment = path.substr(start, end - start);
        if (!segment.empty()) {
            base += "/";
            base += segment;
            if (mkdir(base.c_str(), 0755) != 0 && errno != EEXIST) {
                Log::error("[FileIO] Android: mkdir failed: ", base, " ", std::strerror(errno));
                return false;
            }
        }
        start = end + 1;
    }
    return true;
#else
    std::error_code ec;
    std::filesystem::path p(path);
    std::filesystem::create_directories(p.parent_path(), ec);
    return !ec;
#endif
}

#if SDL2_ENGINE_PLATFORM_ANDROID
void FileIO::setAssetManager(void* assetManager) {
    s_assetManager = static_cast<AAssetManager*>(assetManager);
}

void FileIO::setDataPath(const std::string& dataPath) {
    s_dataPath = dataPath;
}
#endif
