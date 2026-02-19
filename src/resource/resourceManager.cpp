#include "resourceManager.h"
#include "../utils/logger.h"
#include "../utils/FileIO.h"

#include <json/json.h>
#include <sstream>

ResourceManager::ResourceManager(SDL_Renderer* sdlRenderer) 
    : renderer(sdlRenderer) {}

ResourceManager::~ResourceManager() {
    cleanup();
}

void ResourceManager::loadResources(const std::string& resourceFilePath) {
    std::string content = FileIO::readFileAsText(resourceFilePath);
    if (content.empty()) {
        Log::error("Error: Cannot open resource file.");
        return;
    }

    Json::CharReaderBuilder builder;
    Json::Value root;
    std::string errs;
    std::istringstream iss(content);
    if (!Json::parseFromStream(builder, iss, &root, &errs)) {
        Log::error("[ResourceManager] Failed to parse resource file: ", errs);
        return;
    }

    // 초기 씬 이름 읽기
    if (root.isMember("initial_scene") && root["initial_scene"].isString()) {
        initialScene = root["initial_scene"].asString();
        Log::info("[ResourceManager] Initial scene: ", initialScene);
    } else {
        initialScene = "";  // 기본값
        Log::info("[ResourceManager] No initial_scene specified, using default");
    }

    // 리소스 폴더 읽기
    if (root.isMember("resource_folder") && root["resource_folder"].isString()) {
        resourceFolder = root["resource_folder"].asString();
        // 경로 끝에 슬래시가 없으면 추가
        if (!resourceFolder.empty() && resourceFolder.back() != '/') {
            resourceFolder += "/";
        }
        Log::info("[ResourceManager] Resource folder: ", resourceFolder);
    } else {
        resourceFolder = "";  // 기본값 (빈 문자열)
        Log::info("[ResourceManager] No resource_folder specified, using current directory");
    }

    int imagePathCount = 0;
    int sceneCount = 0;
    int jsonCount = 0;
    int textCount = 0;

    for (const auto& res : root["resources"]) {
        std::string name = res["name"].asString();
        std::string path = res["path"].asString();
        std::string type = res["type"].asString();
        
        // resource_folder가 있으면 경로 앞에 붙이기
        std::string fullPath = resourceFolder + path;

        if (type == "image") {
            // 이미지 경로만 저장 (lazy loading)
            imagePaths[name] = fullPath;
            imagePathCount++;
        }
        else if (type == "scene") {
            std::string sceneContent = FileIO::readFileAsText(fullPath);
            if (sceneContent.empty()) {
                Log::error("[ResourceManager] Failed to open scene file: ", fullPath);
                continue;
            }
            try {
                nlohmann::json sceneJson = nlohmann::json::parse(sceneContent);
                scenes[name] = sceneJson;
                sceneCount++;
            } catch (const std::exception& e) {
                Log::error("[ResourceManager] Error parsing scene json: ", fullPath, " - ", e.what());
            }
        }
        else if (type == "json") {
            std::string jsonContent = FileIO::readFileAsText(fullPath);
            if (!jsonContent.empty()) {
                jsonTexts[name] = jsonContent;
                jsonCount++;
            } else {
                Log::error("[ResourceManager] Failed to load json file: ", fullPath);
            }
        }
        else if (type == "text") {
            std::string textContent = FileIO::readFileAsText(fullPath);
            if (!textContent.empty()) {
                texts[name] = textContent;
                textCount++;
            } else {
                Log::error("[ResourceManager] Failed to load text: ", fullPath);
            }
        }
    }

    Log::info("[ResourceManager] Registered ", imagePathCount, " image paths, ", 
              sceneCount, " scenes, ", jsonCount, " json files, and ", 
              textCount, " texts. (Images will be loaded on demand)");
}


SDL_Texture* ResourceManager::getTexture(const std::string& textureName) {
    // 먼저 이미 로드된 텍스처 확인
    auto it = textures.find(textureName);
    if (it != textures.end()) {
        return it->second;
    }
    
    // 이미지 경로에서 찾아서 lazy loading
    auto pathIt = imagePaths.find(textureName);
    if (pathIt != imagePaths.end()) {
        std::string path = pathIt->second;
        
        // 이미지 로드
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) {
            Log::error("[ResourceManager] Error: Unable to load image ", path, 
                      ", SDL_image Error: ", IMG_GetError());
            return nullptr;
        }

        SDL_Texture* sdlTexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);

        if (!sdlTexture) {
            Log::error("[ResourceManager] Error: Unable to create texture for ", 
                      textureName, ", SDL Error: ", SDL_GetError());
            return nullptr;
        }

        // 텍스처 캐싱
        textures[textureName] = sdlTexture;
        return sdlTexture;
    }
    
    // 찾을 수 없음
    return nullptr;
}

std::string ResourceManager::registerTexture(const std::string& textureName, SDL_Texture* texture) {
    if (!texture) {
        std::cerr << "[ResourceManager] Cannot register null texture: " << textureName << std::endl;
        return "";
    }
    
    // 이미 등록되어 있으면 기존 ID 반환 (참조 카운트 증가)
    auto it = textures.find(textureName);
    if (it != textures.end()) {
        // 참조 카운트 증가
        textureRefCount[textureName]++;
        return textureName;  // 기존 ID 반환
    }
    
    // 새로 등록
    textures[textureName] = texture;
    textureRefCount[textureName] = 1;
    return textureName;  // 지정한 이름을 ID로 반환
}

// 이름 없이 등록: 자동 ID 생성
std::string ResourceManager::registerTexture(SDL_Texture* texture) {
    if (!texture) {
        std::cerr << "[ResourceManager] Cannot register null texture" << std::endl;
        return "";
    }
    
    // 고유 ID 생성
    dynamicTextureIdCounter++;
    std::string textureId = "dynamic_" + std::to_string(dynamicTextureIdCounter);
    
    // 등록
    return registerTexture(textureId, texture);
}

void ResourceManager::unregisterTexture(const std::string& textureName) {
    auto refIt = textureRefCount.find(textureName);
    if (refIt == textureRefCount.end()) {
        // 참조 카운트가 없으면 정적 텍스처 (이미지)이므로 무시
        return;
    }
    
    // 참조 카운트 감소
    refIt->second--;
    
    // 참조 카운트가 0이 되면 텍스처 삭제
    if (refIt->second <= 0) {
        auto texIt = textures.find(textureName);
        if (texIt != textures.end()) {
            SDL_DestroyTexture(texIt->second);
            textures.erase(texIt);
        }
        textureRefCount.erase(refIt);
    }
}

nlohmann::json ResourceManager::getSceneJson(const std::string& resourceName) {
    if (scenes.find(resourceName) != scenes.end()) {
        return scenes[resourceName];
    }
    return nullptr;  // 혹은 빈 json 반환
}

nlohmann::json ResourceManager::getJson(const std::string& jsonName) {
    auto it = jsonTexts.find(jsonName);
    if (it == jsonTexts.end()) {
        return nullptr;
    }
    
    // 요청 시 파싱 (lazy loading)
    try {
        nlohmann::json jsonData = nlohmann::json::parse(it->second);
        return jsonData;
    } catch (const std::exception& e) {
        std::cerr << "[ResourceManager] Error parsing JSON: " << jsonName << " - " << e.what() << std::endl;
        return nullptr;
    }
}

nlohmann::json ResourceManager::getMapJson(const std::string& mapName) {
    nlohmann::json jsonData = getJson(mapName);
    if (jsonData.is_null()) {
        return nullptr;
    }
    
    // 타입 확인
    if (!jsonData.contains("type")) {
        std::cerr << "[ResourceManager] JSON file missing 'type' field: " << mapName << std::endl;
        return nullptr;
    }
    
    std::string jsonType = jsonData["type"].get<std::string>();
    if (jsonType != "map") {
        std::cerr << "[ResourceManager] JSON type mismatch. Expected 'map', got: " << jsonType << std::endl;
        return nullptr;
    }
    
    return jsonData;
}

nlohmann::json ResourceManager::getAnimationJson(const std::string& animName) {
    // JSON 파일 텍스트 찾기
    auto it = jsonTexts.find(animName);
    if (it == jsonTexts.end()) {
        std::cerr << "[ResourceManager] Animation JSON not found: " << animName << std::endl;
        return nullptr;
    }
    
    // 요청 시 파싱 (lazy loading)
    try {
        nlohmann::json jsonData = nlohmann::json::parse(it->second);
        
        return jsonData;
    } catch (const std::exception& e) {
        std::cerr << "[ResourceManager] Error parsing animation JSON: " << animName << " - " << e.what() << std::endl;
        return nullptr;
    }
}

std::string ResourceManager::getText(const std::string& name) {
    auto it = texts.find(name);
    return (it != texts.end()) ? it->second : "";
}

SDL_Texture* ResourceManager::createNinePatchTexture(SDL_Texture* sourceTexture, int width, int height) {
    if (!sourceTexture || !renderer || width <= 0 || height <= 0) {
        return nullptr;
    }
    
    // 원본 텍스처 크기 가져오기
    int srcW, srcH;
    SDL_QueryTexture(sourceTexture, nullptr, nullptr, &srcW, &srcH);
    
    if (srcW <= 0 || srcH <= 0) {
        return nullptr;
    }
    
    // 원본 크기 기준으로 9등분 계산 (스케일 다운 전 원본 크기 사용)
    int thirdW = srcW / 3;
    int thirdH = srcH / 3;
    
    Log::info("[ResourceManager] Creating nine-patch: src=", srcW, "x", srcH, 
              ", dst=", width, "x", height, ", third=", thirdW, "x", thirdH);
    
    // 목적지 텍스처 생성
    SDL_Texture* dstTexture = SDL_CreateTexture(renderer, 
                                                SDL_PIXELFORMAT_RGBA32,
                                                SDL_TEXTUREACCESS_TARGET,
                                                width, height);
    if (!dstTexture) {
        Log::error("[ResourceManager] Failed to create nine-patch destination texture");
        return nullptr;
    }
    
    // 투명도 유지를 위한 블렌딩 모드 설정
    SDL_SetTextureBlendMode(dstTexture, SDL_BLENDMODE_BLEND);
    
    // 렌더 타겟 설정
    SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, dstTexture);
    
    // 투명색으로 클리어 (투명도 유지)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    
    // 소스 영역 9개 정의 (정확한 9등분)
    int srcLeftW = thirdW;
    int srcCenterW = thirdW;
    int srcRightW = srcW - thirdW * 2;  // 나머지
    int srcTopH = thirdH;
    int srcCenterH = thirdH;
    int srcBottomH = srcH - thirdH * 2;  // 나머지
    
    SDL_Rect srcRects[9] = {
        {0, 0, srcLeftW, srcTopH},                                    // 좌상
        {srcLeftW, 0, srcCenterW, srcTopH},                         // 상중
        {srcLeftW + srcCenterW, 0, srcRightW, srcTopH},             // 우상
        {0, srcTopH, srcLeftW, srcCenterH},                         // 좌중
        {srcLeftW, srcTopH, srcCenterW, srcCenterH},                // 중앙
        {srcLeftW + srcCenterW, srcTopH, srcRightW, srcCenterH},   // 우중
        {0, srcTopH + srcCenterH, srcLeftW, srcBottomH},            // 좌하
        {srcLeftW, srcTopH + srcCenterH, srcCenterW, srcBottomH},   // 하중
        {srcLeftW + srcCenterW, srcTopH + srcCenterH, srcRightW, srcBottomH} // 우하
    };
    
    // 목적지 영역 9개 계산
    // 모서리는 원본 크기를 유지하되, 목표가 모서리보다 작으면 비율에 맞게 축소
    // 단, 모서리가 너무 작아지지 않도록 최소값 보장
    
    // 모서리 크기 계산: 원본 크기와 목표 크기 중 작은 값 사용
    // 하지만 모서리가 너무 작아지지 않도록 최소값 보장
    int dstLeftW = srcLeftW;
    int dstRightW = srcRightW;
    int dstTopH = srcTopH;
    int dstBottomH = srcBottomH;
    
    // 목표 크기가 모서리보다 작으면 비율에 맞게 축소
    // 하지만 모서리 합이 목표 크기를 넘지 않도록 조정
    if (dstLeftW + dstRightW > width) {
        float scale = static_cast<float>(width) / static_cast<float>(dstLeftW + dstRightW);
        dstLeftW = static_cast<int>(dstLeftW * scale);
        dstRightW = static_cast<int>(dstRightW * scale);
        // 최소값 보장 (최소 1픽셀)
        if (dstLeftW < 1) dstLeftW = 1;
        if (dstRightW < 1) dstRightW = 1;
    }
    
    if (dstTopH + dstBottomH > height) {
        float scale = static_cast<float>(height) / static_cast<float>(dstTopH + dstBottomH);
        dstTopH = static_cast<int>(dstTopH * scale);
        dstBottomH = static_cast<int>(dstBottomH * scale);
        // 최소값 보장 (최소 1픽셀)
        if (dstTopH < 1) dstTopH = 1;
        if (dstBottomH < 1) dstBottomH = 1;
    }
    
    int dstCenterW = width - dstLeftW - dstRightW;  // 중앙 너비: 남은 공간
    int dstCenterH = height - dstTopH - dstBottomH; // 중앙 높이: 남은 공간
    
    // 중앙 영역이 음수가 되지 않도록 조정
    if (dstCenterW < 0) {
        dstCenterW = 0;
        // 모서리 크기 재조정
        dstLeftW = width / 2;
        dstRightW = width - dstLeftW;
    }
    if (dstCenterH < 0) {
        dstCenterH = 0;
        // 모서리 크기 재조정
        dstTopH = height / 2;
        dstBottomH = height - dstTopH;
    }
    
    SDL_Rect dstRects[9] = {
        {0, 0, dstLeftW, dstTopH},                                    // 좌상: 원본 크기
        {dstLeftW, 0, dstCenterW, dstTopH},                         // 상중: 너비만 늘림
        {dstLeftW + dstCenterW, 0, dstRightW, dstTopH},              // 우상: 원본 크기
        {0, dstTopH, dstLeftW, dstCenterH},                         // 좌중: 높이만 늘림
        {dstLeftW, dstTopH, dstCenterW, dstCenterH},                // 중앙: 양방향 늘림
        {dstLeftW + dstCenterW, dstTopH, dstRightW, dstCenterH},    // 우중: 높이만 늘림
        {0, dstTopH + dstCenterH, dstLeftW, dstBottomH},             // 좌하: 원본 크기
        {dstLeftW, dstTopH + dstCenterH, dstCenterW, dstBottomH},   // 하중: 너비만 늘림
        {dstLeftW + dstCenterW, dstTopH + dstCenterH, dstRightW, dstBottomH} // 우하: 원본 크기
    };
    
    // 원본 텍스처의 블렌딩 모드 저장 및 설정 (투명도 유지)
    SDL_BlendMode oldBlendMode;
    SDL_GetTextureBlendMode(sourceTexture, &oldBlendMode);
    SDL_SetTextureBlendMode(sourceTexture, SDL_BLENDMODE_BLEND);
    
    // 9개 영역 렌더링 (원본 텍스처에서 직접)
    int renderedCount = 0;
    for (int i = 0; i < 9; i++) {
        if (srcRects[i].w > 0 && srcRects[i].h > 0 && 
            dstRects[i].w > 0 && dstRects[i].h > 0) {
            SDL_RenderCopy(renderer, sourceTexture, &srcRects[i], &dstRects[i]);
            renderedCount++;
        }
    }
    
    // 원본 텍스처의 블렌딩 모드 복원
    SDL_SetTextureBlendMode(sourceTexture, oldBlendMode);
    Log::info("[ResourceManager] Rendered ", renderedCount, " patches");
    
    // 렌더 타겟 복원
    SDL_SetRenderTarget(renderer, oldTarget);
    
    return dstTexture;
}

SDL_Texture* ResourceManager::createThreePatchTexture(SDL_Texture* sourceTexture, int width, int height) {
    if (!sourceTexture || !renderer || width <= 0 || height <= 0) {
        return nullptr;
    }

    int srcW, srcH;
    SDL_QueryTexture(sourceTexture, nullptr, nullptr, &srcW, &srcH);
    if (srcW <= 0 || srcH <= 0) {
        return nullptr;
    }

    // 가로 3등분: 좌(1/3), 중(1/3), 우(1/3)
    int thirdW = srcW / 3;
    int srcLeftW = thirdW;
    int srcCenterW = thirdW;
    int srcRightW = srcW - thirdW * 2;

    Log::info("[ResourceManager] Creating three-patch (horizontal): src=", srcW, "x", srcH,
              ", dst=", width, "x", height);

    SDL_Texture* dstTexture = SDL_CreateTexture(renderer,
                                                SDL_PIXELFORMAT_RGBA32,
                                                SDL_TEXTUREACCESS_TARGET,
                                                width, height);
    if (!dstTexture) {
        Log::error("[ResourceManager] Failed to create three-patch destination texture");
        return nullptr;
    }

    SDL_SetTextureBlendMode(dstTexture, SDL_BLENDMODE_BLEND);

    SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, dstTexture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    // 좌/우: 원본 비율 유지하며 높이(height)에 맞춰 확대·축소. 가운데만 가변으로 나머지 너비 채움.
    float scaleH = static_cast<float>(height) / static_cast<float>(srcH);
    int dstLeftW = static_cast<int>(srcLeftW * scaleH + 0.5f);
    int dstRightW = static_cast<int>(srcRightW * scaleH + 0.5f);
    if (dstLeftW < 1) dstLeftW = 1;
    if (dstRightW < 1) dstRightW = 1;
    if (dstLeftW + dstRightW > width) {
        float scaleW = static_cast<float>(width) / static_cast<float>(dstLeftW + dstRightW);
        dstLeftW = static_cast<int>(dstLeftW * scaleW + 0.5f);
        dstRightW = static_cast<int>(dstRightW * scaleW + 0.5f);
        if (dstLeftW < 1) dstLeftW = 1;
        if (dstRightW < 1) dstRightW = 1;
    }
    int dstCenterW = width - dstLeftW - dstRightW;
    if (dstCenterW < 0) dstCenterW = 0;

    SDL_Rect srcRects[3] = {
        {0, 0, srcLeftW, srcH},
        {srcLeftW, 0, srcCenterW, srcH},
        {srcLeftW + srcCenterW, 0, srcRightW, srcH}
    };
    SDL_Rect dstRects[3] = {
        {0, 0, dstLeftW, height},
        {dstLeftW, 0, dstCenterW, height},
        {dstLeftW + dstCenterW, 0, dstRightW, height}
    };

    SDL_BlendMode oldBlendMode;
    SDL_GetTextureBlendMode(sourceTexture, &oldBlendMode);
    SDL_SetTextureBlendMode(sourceTexture, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < 3; i++) {
        if (srcRects[i].w > 0 && srcRects[i].h > 0 &&
            dstRects[i].w > 0 && dstRects[i].h > 0) {
            SDL_RenderCopy(renderer, sourceTexture, &srcRects[i], &dstRects[i]);
        }
    }

    SDL_SetTextureBlendMode(sourceTexture, oldBlendMode);
    SDL_SetRenderTarget(renderer, oldTarget);
    return dstTexture;
}

void ResourceManager::cleanup() {
    // 모든 텍스처 정리
    for (auto& texturePair : textures) {
        SDL_DestroyTexture(texturePair.second);
    }
    textures.clear();
    imagePaths.clear();
    textureRefCount.clear();
    
    scenes.clear();
    jsonTexts.clear();
    texts.clear();
}