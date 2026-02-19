#pragma once
#include "../UIWidget.h"
#include <string>
#include <functional>
#include <memory>

class TextRenderer;
class ButtonWidget;
class TextWidget;
class AnimationManager;
class WidgetManager;

class StandardDialogWidget : public UIWidget {
private:
    // 내부 위젯들 (버튼들은 WidgetManager가 소유, TextWidget은 여기서 소유)
    ButtonWidget* okButton;        // WidgetManager가 소유
    ButtonWidget* cancelButton;   // WidgetManager가 소유
    std::unique_ptr<TextWidget> titleWidget;
    std::unique_ptr<TextWidget> descriptionWidget;
    
    // UIElement ID들
    std::string bgElementId;      // 배경 UIElement ID
    std::string iconElementId;    // 아이콘 UIElement ID
    
    // 상태
    bool isVisible;
    std::string currentIconName;
    std::string currentTitle;
    std::string currentDescription;
    
    // 콜백
    std::function<void()> onOkCallback;
    std::function<void()> onCancelCallback;
    
    // Renderer들
    SDL_Renderer* renderer;
    TextRenderer* textRenderer;
    AnimationManager* animationManager;
    WidgetManager* widgetManager;  // 내부 버튼들을 등록하기 위해 필요
    
    // 레이아웃 설정
    void setupLayout();
    void createBackgroundElement();
    void createIconElement();
    void createTitleWidget();
    void createDescriptionWidget();
    void createButtons();
    
    std::string uiElementId;  // UIElement ID (getUIElementIdentifier()에서 사용)
    
public:
    StandardDialogWidget(UiManager* uiMgr, ResourceManager* resMgr,
                        SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                        AnimationManager* animMgr, WidgetManager* wmgr = nullptr);
    
    std::string getUIElementIdentifier() const override { return uiElementId; }
    
    virtual ~StandardDialogWidget();
    
    // 콘텐츠 설정 (아이콘, 타이틀, 설명을 한꺼번에 설정)
    void setContent(const std::string& iconName, const std::string& title, const std::string& description);
    
    // 표시/숨김
    void show();
    void hide();
    bool getVisible() const { return isVisible; }
    
    // 콜백 설정
    void setOnOk(std::function<void()> callback) { onOkCallback = callback; }
    void setOnCancel(std::function<void()> callback) { onCancelCallback = callback; }
};

