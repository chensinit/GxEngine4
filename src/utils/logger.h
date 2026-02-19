#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <sstream>

// 간단한 로그 유틸리티
// 멀티 플랫폼 지원을 위한 기본 구조
class Log {
public:
    // 기본 로그 출력 함수
    static void print(const std::string& message);
    
    // 편의 함수들
    static void info(const std::string& message);
    static void debug(const std::string& message);
    static void error(const std::string& message);
    
    // 스트림 스타일 출력을 위한 헬퍼
    template<typename... Args>
    static void print(Args... args) {
        std::ostringstream oss;
        (oss << ... << args);
        print(oss.str());
    }
    
    template<typename... Args>
    static void info(Args... args) {
        std::ostringstream oss;
        (oss << ... << args);
        info(oss.str());
    }
    
    template<typename... Args>
    static void debug(Args... args) {
        std::ostringstream oss;
        (oss << ... << args);
        debug(oss.str());
    }
    
    template<typename... Args>
    static void error(Args... args) {
        std::ostringstream oss;
        (oss << ... << args);
        error(oss.str());
    }

private:
    // 플랫폼별 출력 구현 (현재는 cout 사용, 나중에 확장 가능)
    static void output(const std::string& message);
};

#endif // LOGGER_H

