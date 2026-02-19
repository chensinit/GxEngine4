#pragma once
#include "../utils/sdl_includes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

#include "../resource/resourceManager.h"
#include "../rendering/TextRenderer.h"
#include <nlohmann/json.hpp>

struct UIElement {
    std::string name;
    SDL_Texture* texture;
    std::string textureId;  // 텍스처 ID (ResourceManager에서 관리)
    SDL_Rect rect;
    float scale = 1.0f;
    float rotation = 0.0f;
    float alpha = 1.0f;
    bool visible = true;
    bool clickable = false;
    bool useNinePatch = false;   // true면 changeTexture 시에도 9패치로 생성
    bool useThreePatch = false;  // true면 changeTexture 시에도 3패치로 생성
    bool alwaysOnTop = false;    // true면 렌더 2패스에서 맨 나중에 그려져 최상위 표시

    // 부모-자식 관계 (다단계: 위치/스케일/회전 상속)
    std::string parentName;  // 부모 이름 (빈 문자열이면 루트)
};

class UiManager {
private:
    std::vector<UIElement> elements;
    std::unordered_map<std::string, size_t> nameIndexMap;
    ResourceManager* resourceManager = nullptr;

    // 베이스 텍스처 + 크기 + 9/3패치 여부 → 사용할 텍스처 생성 (loadUIFromJson / changeTexture 공용)
    SDL_Texture* createTextureForImage(SDL_Texture* baseTexture, int width, int height,
                                        bool useNinePatch, bool useThreePatch,
                                        std::string& outTextureId);
    void renderElement(SDL_Renderer* renderer, const UIElement& element);

public:
    void setResourceManager(ResourceManager* resourceManager);

    UIElement* findElementByName(const std::string& name);
    UIElement* findElementByPosition(int x, int y);

    void addUI(const UIElement& element);
    std::string addUIAndGetId(const UIElement& element);  // name이 비어있으면 자동 ID 생성하여 반환
    bool removeUI(const std::string& name);  // UIElement 제거
    void changeTexture(const std::string& name, const std::string& texture);
    void changeTextureById(const std::string& elementName, const std::string& textureId);
    std::string getTextureId(const std::string& elementName) const;

    void move(const std::string& name, int dx, int dy);
    void moveTo(const std::string& name, int x, int y);
    void resize(const std::string& name, int w, int h);
    void setRect(const std::string& name, int x, int y, int w, int h);

    void setAlpha(const std::string& name, float alpha);
    void setVisible(const std::string& name, bool visible);
    void setAlwaysOnTop(const std::string& name, bool onTop);
    void setRotate(const std::string& name, float angle);
    void setScale(const std::string& name, float scale);

    // getter
    int getLeft(const std::string& name) const;
    int getTop(const std::string& name) const;
    int getRight(const std::string& name) const;
    int getBottom(const std::string& name) const;
    int getWidth(const std::string& name) const;
    int getHeight(const std::string& name) const;

    float getAlpha(const std::string& name) const;
    float getScale(const std::string& name) const;
    float getRotate(const std::string& name) const;

    bool isVisible(const std::string& name) const;
    
    // 부모-자식 관계 관리 (다단계: 위치/스케일/회전 상속)
    void setParent(const std::string& childName, const std::string& parentName);
    void removeParent(const std::string& childName);
    
    // 순환 참조 체크: childName이 parentName의 조상인지 확인
    bool wouldCreateCycle(const std::string& childName, const std::string& parentName) const;
    
    // 월드 위치 계산 (부모 위치 누적)
    void getWorldPosition(const std::string& elementName, int& worldX, int& worldY) const;
    
    // 스케일 적용된 렌더링 위치와 크기 계산 (중심 기준)
    void getScaledRect(const std::string& elementName, SDL_Rect& scaledRect) const;
    
    // 스케일 적용된 중심점 계산
    void getScaledCenter(const std::string& elementName, int& centerX, int& centerY) const;
    
    // 월드 회전 계산 (부모 회전 누적)
    float getWorldRotation(const std::string& elementName) const;
    
    std::vector<UIElement>& getElements();
    const std::vector<UIElement>& getElements() const;
    void clear();
    
    // 렌더링
    void render(SDL_Renderer* renderer);
    
    // JSON에서 UI 요소 로딩
    bool loadUIFromJson(const nlohmann::json& uiElement, 
                        SDL_Renderer* renderer,
                        TextRenderer* textRenderer);

};
