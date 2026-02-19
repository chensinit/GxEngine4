#pragma once

#include "platform_config.h"
#include <string>

/**
 * 플랫폼 공통 파일 I/O.
 * PC: std::ifstream / std::ofstream
 * Android: AAssetManager(읽기), 내부 저장소(쓰기)
 * 분기는 FileIO.cpp 한 곳에서만 처리.
 */
class FileIO {
public:
    /** 경로의 파일 전체를 텍스트로 읽기. 실패 시 빈 문자열 */
    static std::string readFileAsText(const std::string& path);

    /** 경로에 텍스트 덮어쓰기. 디렉터리는 자동 생성하지 않음 */
    static bool writeFileAsText(const std::string& path, const std::string& content);

    /** path에 해당하는 부모 디렉터리 생성 (파일 쓰기 전에 호출 가능) */
    static bool createDirectoriesForFile(const std::string& path);

#if SDL2_ENGINE_PLATFORM_ANDROID
    /** Android: APK assets 읽기용. 앱 시작 시 한 번 설정 */
    static void setAssetManager(void* assetManager);

    /** Android: 쓰기 시 사용할 내부 저장소 기준 경로 (예: /data/data/패키지/files) */
    static void setDataPath(const std::string& dataPath);
#endif
};
