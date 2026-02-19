#pragma once
#include "../UIWidget.h"
#include <string>
#include <memory>
#include "../../utils/sdl_includes.h"

class TextRenderer;
class AnimationManager;

class ToastWidget : public UIWidget {
private:
    std::string uiElementId;
    std::string textElementId;
    std::unique_ptr<class TextWidget> textWidget;
    
    SDL_Renderer* renderer;
    TextRenderer* textRenderer;
    AnimationManager* animationManager;
    
    int fadeInDuration;
    int fadeOutDuration;
    int displayDuration;
    int defaultFontSize;
    SDL_Color defaultTextColor;
    
    void createTextWidget(const std::string& text, int fontSize, SDL_Color textColor, const SDL_Rect& rect);
    
public:
    ToastWidget(UiManager* uiMgr, ResourceManager* resMgr,
                SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                AnimationManager* animMgr,
                const SDL_Rect& rect, int fontSize = 18, SDL_Color textColor = {0, 0, 0, 255});
    
    virtual ~ToastWidget();
    
    std::string getUIElementIdentifier() const override { return uiElementId; }
    
    void showToast(const std::string& text, int displayDurationMs = 3000);
    void hide();
    bool isVisible() const;
};
