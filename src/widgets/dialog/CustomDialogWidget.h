#pragma once
#include "../UIWidget.h"
#include <string>

class AnimationManager;

class CustomDialogWidget : public UIWidget {
private:
    // UIElement ID
    std::string bgElementId;      // 배경 UIElement ID
    
    // 상태
    bool isVisible;
    
    // Renderer들
    SDL_Renderer* renderer;
    AnimationManager* animationManager;
    
    // 배경 UIElement 생성
    void createBackgroundElement(const std::string& widgetName, const std::string& imageName, const SDL_Rect& rect);
    
    std::string uiElementId;  // UIElement ID (getUIElementIdentifier()에서 사용)
    
public:
    CustomDialogWidget(UiManager* uiMgr, ResourceManager* resMgr,
                      SDL_Renderer* sdlRenderer, AnimationManager* animMgr,
                      const std::string& widgetName, const std::string& imageName, const SDL_Rect& rect);
    
    std::string getUIElementIdentifier() const override { return uiElementId; }
    
    virtual ~CustomDialogWidget();
    
    // 표시/숨김
    void show();
    void hide();
    void dismiss();  // hide()와 동일 (이름만 다름)
    bool getVisible() const { return isVisible; }
};

