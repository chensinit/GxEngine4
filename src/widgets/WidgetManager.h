#pragma once
#include <memory>
#include <unordered_map>
#include <map>
#include <string>
#include <vector>
#include "../utils/sdl_includes.h"
#include <nlohmann/json.hpp>
#include "UIWidget.h"  // 같은 폴더

class UiManager;
class ResourceManager;
class TextRenderer;
class AnimationManager;

class WidgetManager {
private:
    std::unordered_map<std::string, std::unique_ptr<UIWidget>> widgets;
    UiManager* uiManager;           // UIElement 제어용
    ResourceManager* resourceManager; // 리소스 접근용
    SDL_Renderer* renderer;          // 텍스처 생성용
    TextRenderer* textRenderer;       // 텍스트 렌더링용
    AnimationManager* animationManager; // 애니메이션 관리용
    UIWidget* focusedWidget;         // 현재 포커스를 가진 위젯 (EditText 등)
    
    // 포커스/클릭 관련 헬퍼 함수
    UIWidget* findWidgetAtPosition(int x, int y, bool focusableOnly = false);
    
    // JSON에서 UIElement 속성 적용 헬퍼 함수
    void applyElementProperties(struct UIElement* element, const nlohmann::json& uiElement);
    void applyParentRelationship(const std::string& elementId, const nlohmann::json& uiElement);
    
public:
    WidgetManager(UiManager* uiMgr, ResourceManager* resMgr, 
                 SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                 AnimationManager* animMgr = nullptr);
    
    // 위젯 관리
    void addWidget(const std::string& name, std::unique_ptr<UIWidget> widget);
    UIWidget* getWidget(const std::string& name);
    void removeWidget(const std::string& name);
    void clear();  // 씬 변경 시 모든 위젯 제거
    
    // 업데이트 및 렌더링
    void update(float deltaTime);
    void handleEvents(const std::vector<SDL_Event>& events);
    void render(SDL_Renderer* renderer);  // 필요시 (대부분은 UIElement로 렌더링)
    
    // UiManager 접근 (위젯이 UIElement 제어할 때 필요)
    UiManager* getUiManager() { return uiManager; }
    ResourceManager* getResourceManager() { return resourceManager; }
    
    // Renderer 접근 (위젯이 사용)
    SDL_Renderer* getRenderer() { return renderer; }
    TextRenderer* getTextRenderer() { return textRenderer; }
    
    // JSON에서 위젯 로딩
    bool loadWidgetFromJson(const nlohmann::json& uiElement);
    
    // renderer/textRenderer/animationManager 설정 (필요시)
    void setRenderer(SDL_Renderer* sdlRenderer) { renderer = sdlRenderer; }
    void setTextRenderer(TextRenderer* txtRenderer) { textRenderer = txtRenderer; }
    void setAnimationManager(AnimationManager* animMgr) { animationManager = animMgr; }
    
    // 포커스 관리
    void setFocusedWidget(UIWidget* widget);
    UIWidget* getFocusedWidget() const { return focusedWidget; }
    void clearFocus();
};

