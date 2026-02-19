#include "logger.h"
#include <iostream>

void Log::print(const std::string& message) {
    output(message);
}

void Log::info(const std::string& message) {
    output("[INFO] " + message);
}

void Log::debug(const std::string& message) {
    output("[DEBUG] " + message);
}

void Log::error(const std::string& message) {
    output("[ERROR] " + message);
}

void Log::output(const std::string& message) {
    // 현재는 cout 사용
    // 나중에 플랫폼별로 분리 가능 (예: Android Log, iOS NSLog 등)
    std::cout << message << std::endl;
}

