#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include "../utils/sdl_includes.h"
#include <string>
#include <map>
#include <unordered_map>
#include <nlohmann/json.hpp>

class ResourceManager {
private:
    SDL_Renderer* renderer;
    std::map<std::string, SDL_Texture*> textures;  // 로드된 텍스처 (이미지 + 동적 텍스처)
    std::map<std::string, std::string> imagePaths;   // 이미지 경로 (lazy loading용)
    std::map<std::string, int> textureRefCount;     // 참조 카운팅 (동적 텍스처용)
    std::unordered_map<std::string, nlohmann::json> scenes;  // 씬 데이터 (중요하므로 즉시 파싱)
    std::map<std::string, std::string> jsonTexts;   // JSON 파일 텍스트 (lazy loading용)
    std::map<std::string, std::string> texts;       // 일반 텍스트 파일
    std::string initialScene;                        // 초기 씬 이름
    std::string resourceFolder;                     // 리소스 기본 폴더
    int dynamicTextureIdCounter = 0;                 // 동적 텍스처 ID 생성용 카운터

public:
    ResourceManager(SDL_Renderer* sdlRenderer);
    ~ResourceManager();

    void loadResources(const std::string& resourceFilePath);
    SDL_Texture* getTexture(const std::string& textureName);
    nlohmann::json getSceneJson(const std::string& resourceName);
    nlohmann::json getMapJson(const std::string& mapName);  // 요청 시 파싱
    nlohmann::json getJson(const std::string& jsonName);    // 범용 JSON 로더
    nlohmann::json getAnimationJson(const std::string& animName);  // 애니메이션 JSON 로더 (배열 형태)
    std::string getText(const std::string& name);
    std::string getInitialScene() const { return initialScene; }  // 초기 씬 이름 반환
    
    // 텍스처 등록/해제 (동적 텍스처용 - 텍스트 등)
    // 이름을 지정한 경우: 지정한 이름이 ID가 되어 반환됨
    // ⚠️ 왠만하면 쓰지 말 것! 대부분의 경우 자동 ID 생성 버전을 사용하세요.
    // 이름 지정은 JSON에서 로드한 정적 텍스처를 참조할 때만 필요합니다.
    std::string registerTexture(const std::string& textureName, SDL_Texture* texture);
    // 이름 없이 등록: 자동으로 고유 ID 생성하여 반환 (권장)
    std::string registerTexture(SDL_Texture* texture);
    void unregisterTexture(const std::string& textureId);
    
    // 9패치 텍스처 생성 (원본 텍스처를 9등분해서 목적지 크기로 확장)
    SDL_Texture* createNinePatchTexture(SDL_Texture* sourceTexture, int width, int height);

    // 3패치 텍스처 생성 (원본을 가로 3등분, 좌/우는 고정·중앙만 가로로 늘림, 세로는 전체 늘림)
    SDL_Texture* createThreePatchTexture(SDL_Texture* sourceTexture, int width, int height);
    
    void cleanup();
};

#endif