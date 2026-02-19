#include "uiManager.h"
#include "../utils/logger.h"
#include "../rendering/ImageRenderer.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


void UiManager::setResourceManager(ResourceManager* resourceManager) {
    this->resourceManager = resourceManager;
    // textRenderer.init("data/Jalnan2.otf")
}

UIElement* UiManager::findElementByName(const std::string& name) {
    for (auto& element : elements) {
        if (element.name == name) {
            return &element;
        }
    }
    return nullptr;
}

UIElement* UiManager::findElementByPosition(int x, int y) {
    SDL_Point p{ x, y };
    // 나중에 추가된 요소가 위에 있다고 가정 → 역순 순회
    for (auto it = elements.rbegin(); it != elements.rend(); ++it) {
        if (!it->visible || !it->clickable) continue; // 보이지 않으면 스킵 (원하면 제거 가능)
        if (SDL_PointInRect(&p, &it->rect)) {
            return &(*it);
        }
    }
    return nullptr;
}

void UiManager::addUI(const UIElement& element) {
    nameIndexMap[element.name] = elements.size();
    elements.push_back(element);
}

std::string UiManager::addUIAndGetId(const UIElement& element) {
    UIElement newElement = element;
    
    // name이 비어있으면 자동 ID 생성
    if (newElement.name.empty()) {
        static int elementIdCounter = 0;
        elementIdCounter++;
        newElement.name = "element_" + std::to_string(elementIdCounter);
    }
    
    addUI(newElement);
    return newElement.name;
}

bool UiManager::removeUI(const std::string& name) {
    // nameIndexMap에서 찾기
    auto it = nameIndexMap.find(name);
    if (it == nameIndexMap.end()) {
        return false;  // 요소를 찾을 수 없음
    }
    
    size_t index = it->second;
    
    // elements에서 제거
    if (index < elements.size()) {
        elements.erase(elements.begin() + index);
        
        // nameIndexMap 재구성 (제거된 요소 이후의 인덱스들을 1씩 감소)
        nameIndexMap.clear();
        for (size_t i = 0; i < elements.size(); i++) {
            nameIndexMap[elements[i].name] = i;
        }
        
        return true;
    }
    
    return false;
}


// 베이스 텍스처 + 크기 + 9/3패치 여부 → 사용할 텍스처 생성 (등록 시 outTextureId 설정, 미등록이면 빈 문자열)
SDL_Texture* UiManager::createTextureForImage(SDL_Texture* baseTexture, int width, int height,
                                               bool useNinePatch, bool useThreePatch,
                                               std::string& outTextureId) {
    outTextureId.clear();
    if (!baseTexture || width <= 0 || height <= 0) return baseTexture;

    if (useNinePatch) {
        SDL_Texture* tex = resourceManager->createNinePatchTexture(baseTexture, width, height);
        if (tex) {
            outTextureId = resourceManager->registerTexture(tex);
            return tex;
        }
        return baseTexture;
    }
    if (useThreePatch) {
        SDL_Texture* tex = resourceManager->createThreePatchTexture(baseTexture, width, height);
        if (tex) {
            outTextureId = resourceManager->registerTexture(tex);
            return tex;
        }
        return baseTexture;
    }
    return baseTexture;
}

// use by script
void UiManager::changeTexture(const std::string& elementName, const std::string& imageName) {
    UIElement* element = findElementByName(elementName);
    if (!element) {
        Log::error("[UI] Element not found: ", elementName);
        return;
    }

    SDL_Texture* baseTexture = resourceManager->getTexture(imageName);
    if (!baseTexture) {
        Log::error("[UI] Texture not found for image: ", imageName);
        return;
    }

    int w = element->rect.w;
    int h = element->rect.h;
    bool usePatch = element->useNinePatch || element->useThreePatch;

    if (usePatch && w > 0 && h > 0) {
        if (!element->textureId.empty() && element->textureId.substr(0, 8) == "dynamic_") {
            resourceManager->unregisterTexture(element->textureId);
        }
        std::string newId;
        SDL_Texture* tex = createTextureForImage(baseTexture, w, h, element->useNinePatch, element->useThreePatch, newId);
        element->texture = tex;
        element->textureId = newId.empty() ? imageName : newId;
    } else {
        element->texture = baseTexture;
        element->textureId = imageName;
    }
}

// 텍스처 ID로 변경
void UiManager::changeTextureById(const std::string& elementName, const std::string& textureId) {
    UIElement* element = findElementByName(elementName);
    if (!element) {
        Log::error("[UiManager] Element not found: ", elementName);
        return;
    }
    
    if (!resourceManager) {
        Log::error("[UiManager] ResourceManager not set");
        return;
    }
    
    SDL_Texture* texture = resourceManager->getTexture(textureId);
    if (!texture) {
        Log::error("[UiManager] Texture not found for ID: ", textureId);
        return;
    }
    
    element->texture = texture;
    element->textureId = textureId;
}

// 텍스처 ID 가져오기
std::string UiManager::getTextureId(const std::string& elementName) const {
    UIElement* element = const_cast<UiManager*>(this)->findElementByName(elementName);
    if (!element) {
        return "";
    }
    return element->textureId;
}


void UiManager::move(const std::string& name, int dx, int dy) {
    if (nameIndexMap.find(name) == nameIndexMap.end()) return;
    auto& rect = elements[nameIndexMap[name]].rect;
    rect.x += dx;
    rect.y += dy;
}

void UiManager::moveTo(const std::string& name, int x, int y) {
    if (nameIndexMap.find(name) == nameIndexMap.end()) return;
    auto& rect = elements[nameIndexMap[name]].rect;
    rect.x = x;
    rect.y = y;
}

void UiManager::resize(const std::string& name, int w, int h) {
    if (nameIndexMap.find(name) == nameIndexMap.end()) return;
    auto& rect = elements[nameIndexMap[name]].rect;
    rect.w = w;
    rect.h = h;
}

void UiManager::setRect(const std::string& name, int x, int y, int w, int h) {
    if (nameIndexMap.find(name) == nameIndexMap.end()) return;
    auto& rect = elements[nameIndexMap[name]].rect;
    rect = {x, y, w, h};
}

void UiManager::setAlpha(const std::string& name, float alpha) {
    if (nameIndexMap.find(name) == nameIndexMap.end()) return;
    elements[nameIndexMap[name]].alpha = alpha;
}

void UiManager::setVisible(const std::string& name, bool visible) {
    if (nameIndexMap.find(name) == nameIndexMap.end()) return;
    elements[nameIndexMap[name]].visible = visible;
}

void UiManager::setAlwaysOnTop(const std::string& name, bool onTop) {
    UIElement* el = findElementByName(name);
    if (el) el->alwaysOnTop = onTop;
}

void UiManager::setRotate(const std::string& name, float angle) {
    if (nameIndexMap.find(name) == nameIndexMap.end()) return;
    elements[nameIndexMap[name]].rotation = angle;
}

void UiManager::setScale(const std::string& name, float scale) {
    if (nameIndexMap.find(name) == nameIndexMap.end()) return;
    elements[nameIndexMap[name]].scale = scale;
}


// getter
int UiManager::getLeft(const std::string& name) const {
    auto it = nameIndexMap.find(name);
    if (it == nameIndexMap.end()) return 0;
    return elements[it->second].rect.x;
}

int UiManager::getTop(const std::string& name) const {
    auto it = nameIndexMap.find(name);
    if (it == nameIndexMap.end()) return 0;
    return elements[it->second].rect.y;
}

int UiManager::getRight(const std::string& name) const {
    auto it = nameIndexMap.find(name);
    if (it == nameIndexMap.end()) return 0;
    const auto& rect = elements[it->second].rect;
    return rect.x + rect.w;
}

int UiManager::getBottom(const std::string& name) const {
    auto it = nameIndexMap.find(name);
    if (it == nameIndexMap.end()) return 0;
    const auto& rect = elements[it->second].rect;
    return rect.y + rect.h;
}

int UiManager::getWidth(const std::string& name) const {
    auto it = nameIndexMap.find(name);
    if (it == nameIndexMap.end()) return 0;
    return elements[it->second].rect.w;
}

int UiManager::getHeight(const std::string& name) const {
    auto it = nameIndexMap.find(name);
    if (it == nameIndexMap.end()) return 0;
    return elements[it->second].rect.h;
}

float UiManager::getAlpha(const std::string& name) const {
    auto it = nameIndexMap.find(name);
    if (it == nameIndexMap.end()) return 1.0f;
    return elements[it->second].alpha;
}

float UiManager::getScale(const std::string& name) const {
    auto it = nameIndexMap.find(name);
    if (it == nameIndexMap.end()) return 1.0f;
    return elements[it->second].scale;
}

float UiManager::getRotate(const std::string& name) const {
    auto it = nameIndexMap.find(name);
    if (it == nameIndexMap.end()) return 0.0f;
    return elements[it->second].rotation;
}

bool UiManager::isVisible(const std::string& name) const {
    auto it = nameIndexMap.find(name);
    if (it == nameIndexMap.end()) return false;
    return elements[it->second].visible;
}



std::vector<UIElement>& UiManager::getElements() {
    return elements;
}

const std::vector<UIElement>& UiManager::getElements() const {
    return elements;
}
 
// 순환 참조 체크: childName이 parentName의 조상인지 확인
bool UiManager::wouldCreateCycle(const std::string& childName, const std::string& parentName) const {
    if (childName == parentName) {
        return true;  // 자기 자신을 부모로 설정 불가
    }
    
    if (parentName.empty()) {
        return false;  // 루트로 설정하는 것은 항상 가능
    }
    
    // parentName의 모든 조상을 따라가면서 childName과 일치하는지 확인
    std::string currentParent = parentName;
    while (!currentParent.empty()) {
        if (currentParent == childName) {
            return true;  // 순환 참조 발견
        }
        UIElement* current = const_cast<UiManager*>(this)->findElementByName(currentParent);
        if (!current) {
            break;  // 부모를 찾을 수 없으면 중단
        }
        currentParent = current->parentName;
    }
    
    return false;  // 순환 참조 없음
}

// 부모-자식 관계 관리 (다단계: 위치/스케일/회전 상속)
void UiManager::setParent(const std::string& childName, const std::string& parentName) {
    UIElement* child = findElementByName(childName);
    if (!child) {
        Log::error("[UI] Child element not found: ", childName);
        return;
    }
    
    // 순환 참조 체크
    if (wouldCreateCycle(childName, parentName)) {
        Log::error("[UI] Cannot set parent: would create cycle. Child: ", childName, 
                  ", Parent: ", parentName);
        return;
    }
    
    // 부모가 존재하는지 확인
    if (!parentName.empty()) {
        UIElement* parent = findElementByName(parentName);
        if (!parent) {
            Log::error("[UI] Parent element not found: ", parentName);
            return;
        }
    }
    
    // 기존 부모 제거
    if (!child->parentName.empty()) {
        removeParent(childName);
    }
    
    // 새 부모 설정
    child->parentName = parentName;
}

void UiManager::removeParent(const std::string& childName) {
    UIElement* child = findElementByName(childName);
    if (!child) {
        return;
    }
    
    child->parentName = "";
}

// 월드 위치 계산 (원본 위치, 스케일 미적용)
void UiManager::getWorldPosition(const std::string& elementName, int& worldX, int& worldY) const {
    UIElement* element = const_cast<UiManager*>(this)->findElementByName(elementName);
    if (!element) {
        worldX = 0;
        worldY = 0;
        return;
    }
    
    // 루트: 로컬 위치 그대로
    if (element->parentName.empty()) {
        worldX = element->rect.x;
        worldY = element->rect.y;
        return;
    }
    
    // 자식: 부모의 원본 월드 위치 + 자식 로컬 위치
    UIElement* parent = const_cast<UiManager*>(this)->findElementByName(element->parentName);
    if (parent) {
        int parentWorldX, parentWorldY;
        getWorldPosition(element->parentName, parentWorldX, parentWorldY);
        worldX = parentWorldX + element->rect.x;
        worldY = parentWorldY + element->rect.y;
    } else {
        worldX = element->rect.x;
        worldY = element->rect.y;
    }
}

// 스케일 적용된 중심점 계산
void UiManager::getScaledCenter(const std::string& elementName, int& centerX, int& centerY) const {
    UIElement* element = const_cast<UiManager*>(this)->findElementByName(elementName);
    if (!element) {
        centerX = 0;
        centerY = 0;
        return;
    }
    
    // 월드 스케일 계산 (자신의 scale * 부모의 scale)
    float worldScale = element->scale;
    if (!element->parentName.empty()) {
        UIElement* parentElement = const_cast<UiManager*>(this)->findElementByName(element->parentName);
        if (parentElement) {
            worldScale *= parentElement->scale;
        }
    }
    
    // 원본 월드 위치
    int worldX, worldY;
    getWorldPosition(elementName, worldX, worldY);
    
    // 원본 중심점
    int originalCenterX = worldX + element->rect.w / 2;
    int originalCenterY = worldY + element->rect.h / 2;
    
    // 중심점은 스케일과 무관하게 동일
    centerX = originalCenterX;
    centerY = originalCenterY;
}

// 월드 회전 계산 (부모 회전 누적)
float UiManager::getWorldRotation(const std::string& elementName) const {
    UIElement* element = const_cast<UiManager*>(this)->findElementByName(elementName);
    if (!element) {
        return 0.0f;
    }
    
    // 자신의 회전
    float worldRotation = element->rotation;
    
    // 부모 회전 누적
    if (!element->parentName.empty()) {
        UIElement* parentElement = const_cast<UiManager*>(this)->findElementByName(element->parentName);
        if (parentElement) {
            worldRotation += getWorldRotation(element->parentName);
        }
    }
    
    return worldRotation;
}

// 스케일 적용된 렌더링 위치와 크기 계산 (중심 기준, 부모 회전 고려)
void UiManager::getScaledRect(const std::string& elementName, SDL_Rect& scaledRect) const {
    UIElement* element = const_cast<UiManager*>(this)->findElementByName(elementName);
    if (!element) {
        scaledRect = {0, 0, 0, 0};
        return;
    }
    
    // 월드 스케일 계산 (자신의 scale * 부모의 scale)
    float worldScale = element->scale;
    float parentScale = 1.0f;
    float parentRotation = 0.0f;
    if (!element->parentName.empty()) {
        UIElement* parentElement = const_cast<UiManager*>(this)->findElementByName(element->parentName);
        if (parentElement) {
            parentScale = parentElement->scale;
            worldScale *= parentScale;
            parentRotation = getWorldRotation(element->parentName);
        }
    }
    
    // 스케일된 크기
    int scaledWidth = static_cast<int>(element->rect.w * worldScale);
    int scaledHeight = static_cast<int>(element->rect.h * worldScale);
    
    // 중심점 계산
    int centerX, centerY;
    
    if (!element->parentName.empty()) {
        // 자식: 부모의 스케일된 중심점 기준으로 계산
        UIElement* parentElement = const_cast<UiManager*>(this)->findElementByName(element->parentName);
        if (parentElement) {
            // 부모의 스케일된 중심점 가져오기
            int parentCenterX, parentCenterY;
            getScaledCenter(element->parentName, parentCenterX, parentCenterY);
            
            // 자식의 부모 중심으로부터의 상대 위치 (부모 원본 크기 기준)
            // 자식 로컬 위치는 부모 원본 rect 기준이므로, 부모 중심으로부터의 오프셋 계산
            // 자식의 중심점 로컬 좌표
            int childCenterLocalX = element->rect.x + element->rect.w / 2;
            int childCenterLocalY = element->rect.y + element->rect.h / 2;
            
            // 부모 중심점 로컬 좌표
            int parentCenterLocalX = parentElement->rect.w / 2;
            int parentCenterLocalY = parentElement->rect.h / 2;
            
            // 자식 중심점의 부모 중심점으로부터의 오프셋
            int childOffsetFromParentCenterX = childCenterLocalX - parentCenterLocalX;
            int childOffsetFromParentCenterY = childCenterLocalY - parentCenterLocalY;
            
            // 부모 스케일을 적용한 자식의 오프셋
            float scaledOffsetX = childOffsetFromParentCenterX * parentScale;
            float scaledOffsetY = childOffsetFromParentCenterY * parentScale;
            
            // 부모 회전을 적용한 자식의 오프셋 (회전 변환)
            if (parentRotation != 0.0f) {
                float rad = parentRotation * M_PI / 180.0f;
                float cosR = std::cos(rad);
                float sinR = std::sin(rad);
                float rotatedX = scaledOffsetX * cosR - scaledOffsetY * sinR;
                float rotatedY = scaledOffsetX * sinR + scaledOffsetY * cosR;
                scaledOffsetX = rotatedX;
                scaledOffsetY = rotatedY;
            }
            
            // 자식의 중심점 = 부모 중심 + 회전된 스케일된 오프셋
            centerX = parentCenterX + static_cast<int>(scaledOffsetX);
            centerY = parentCenterY + static_cast<int>(scaledOffsetY);
        } else {
            // 부모를 찾을 수 없으면 루트처럼 처리
            getScaledCenter(elementName, centerX, centerY);
        }
    } else {
        // 루트: 원본 중심점 사용
        getScaledCenter(elementName, centerX, centerY);
    }
    
    // 중심 기준으로 위치 계산
    scaledRect.x = centerX - scaledWidth / 2;
    scaledRect.y = centerY - scaledHeight / 2;
    scaledRect.w = scaledWidth;
    scaledRect.h = scaledHeight;
}

void UiManager::clear() {
    elements.clear();
    nameIndexMap.clear();
}

bool UiManager::loadUIFromJson(const nlohmann::json& uiElement,
                                 SDL_Renderer* renderer,
                                 TextRenderer* textRenderer) {
    std::string type = uiElement["type"].get<std::string>();
    std::string uiName = uiElement["name"].get<std::string>();
    std::vector<int> loc = uiElement["loc"].get<std::vector<int>>();
    
    SDL_Texture* texture = nullptr;
    std::string imageName;
    bool useNinePatch = false;
    bool useThreePatch = false;

    if (type == "image") {
        int elementWidth = loc[2] - loc[0];
        int elementHeight = loc[3] - loc[1];
        if (elementWidth <= 0 || elementHeight <= 0) {
            Log::error("[UiManager] Invalid image size for element: ", uiName);
            return false;
        }

        imageName = uiElement["image"].get<std::string>();
        SDL_Texture* baseTexture = resourceManager->getTexture(imageName);
        if (!baseTexture) {
            Log::error("Texture not found: ", imageName);
            return false;
        }
        
        if (uiElement.contains("useNinePatch")) {
            useNinePatch = uiElement["useNinePatch"].get<bool>();
        } else {
            useNinePatch = (imageName.find("_9patch") != std::string::npos) ||
                          (imageName.find("_9p") != std::string::npos);
        }
        if (uiElement.contains("useThreePatch")) {
            useThreePatch = uiElement["useThreePatch"].get<bool>();
        }
        if (useNinePatch && useThreePatch) {
            useThreePatch = false;  // 9패치 우선
        }

        std::string patchTextureId;
        texture = createTextureForImage(baseTexture, elementWidth, elementHeight, useNinePatch, useThreePatch, patchTextureId);
        if (!patchTextureId.empty()) {
            imageName = patchTextureId;
        }
    }
    else {
        // image가 아닌 타입은 처리하지 않음 (text는 WidgetManager에서 처리)
        return false;
    }
    
    // UIElement 구조체 생성
    UIElement element;
    element.name = uiName;
    element.texture = texture;
    element.textureId = imageName;  // 이미지 이름 또는 텍스처 ID
    element.rect = SDL_Rect{loc[0], loc[1], loc[2] - loc[0], loc[3] - loc[1]};
    element.useNinePatch = useNinePatch;
    element.useThreePatch = useThreePatch;
    
    // 선택적 필드 처리 (기본값 있음)
    if (uiElement.contains("scale")) {
        element.scale = uiElement["scale"].get<float>();
    }
    if (uiElement.contains("rotate")) {
        element.rotation = uiElement["rotate"].get<float>();
    }
    if (uiElement.contains("alpha")) {
        element.alpha = uiElement["alpha"].get<float>();
    }
    if (uiElement.contains("visible")) {
        element.visible = uiElement["visible"].get<bool>();
    }
    if (uiElement.contains("clickable")) {
        element.clickable = uiElement["clickable"].get<bool>();
    }
    
    // 부모-자식 관계 (다단계: 위치/스케일/회전 상속)
    if (uiElement.contains("parent")) {
        element.parentName = uiElement["parent"].get<std::string>();
    }
    
    addUI(element);
    
    // 부모 설정 (addUI 후에 해야 함)
    if (!element.parentName.empty()) {
        setParent(uiName, element.parentName);
    }
    
    return true;
}

void UiManager::renderElement(SDL_Renderer* renderer, const UIElement& element) {
    if (!element.visible || !element.texture) return;

    bool allParentsVisible = true;
    std::string currentParent = element.parentName;
    while (!currentParent.empty()) {
        UIElement* parent = findElementByName(currentParent);
        if (!parent || !parent->visible) {
            allParentsVisible = false;
            break;
        }
        currentParent = parent->parentName;
    }
    if (!allParentsVisible) return;

    SDL_Rect dst;
    getScaledRect(element.name, dst);
    float worldRotation = getWorldRotation(element.name);
    float worldScale = element.scale;
    bool hasParentScaleOrRotation = false;
    if (!element.parentName.empty()) {
        UIElement* parentElement = findElementByName(element.parentName);
        if (parentElement) {
            worldScale *= parentElement->scale;
            if (parentElement->scale != 1.0f || parentElement->rotation != 0.0f) {
                hasParentScaleOrRotation = true;
            }
        }
    }
    bool shouldClip = !(element.scale != 1.0f || element.rotation != 0.0f || hasParentScaleOrRotation);
    bool wasClipped = false;
    if (!element.parentName.empty() && shouldClip) {
        SDL_Rect clipRect = {0, 0, 0, 0};
        bool hasClipRect = false;
        std::string currentParent = element.parentName;
        while (!currentParent.empty()) {
            UIElement* parentElement = findElementByName(currentParent);
            if (!parentElement || !parentElement->visible) break;
            SDL_Rect parentRect;
            getScaledRect(currentParent, parentRect);
            if (!hasClipRect) {
                clipRect = parentRect;
                hasClipRect = true;
            } else {
                SDL_Rect intersection;
                if (SDL_IntersectRect(&clipRect, &parentRect, &intersection)) {
                    clipRect = intersection;
                } else {
                    hasClipRect = false;
                    break;
                }
            }
            currentParent = parentElement->parentName;
        }
        if (hasClipRect) {
            SDL_Rect elementIntersection;
            if (!SDL_IntersectRect(&dst, &clipRect, &elementIntersection)) return;
            SDL_RenderSetClipRect(renderer, &clipRect);
            wasClipped = true;
        } else {
            return;
        }
    }
    float effectiveAlpha = element.alpha;
    std::string alphaParent = element.parentName;
    while (!alphaParent.empty()) {
        UIElement* p = findElementByName(alphaParent);
        if (p) effectiveAlpha *= p->alpha;
        else break;
        alphaParent = p->parentName;
    }
    SDL_SetTextureAlphaMod(element.texture, static_cast<Uint8>(effectiveAlpha * 255));
    SDL_RenderCopyEx(renderer, element.texture, nullptr, &dst, worldRotation, nullptr, SDL_FLIP_NONE);
    if (wasClipped) {
        SDL_RenderSetClipRect(renderer, nullptr);
    }
}

void UiManager::render(SDL_Renderer* renderer) {
    // 1패스: alwaysOnTop 아닌 요소
    for (const auto& element : elements) {
        if (element.alwaysOnTop) continue;
        renderElement(renderer, element);
    }
    // 2패스: alwaysOnTop 요소 (토스트 등 최상위)
    for (const auto& element : elements) {
        if (!element.alwaysOnTop) continue;
        renderElement(renderer, element);
    }
}


