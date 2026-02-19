#pragma once

#include "../UIWidget.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <set>
#include "../../utils/sdl_includes.h"

class TextRenderer;
class BackgroundTextWidget;
class WidgetManager;
class AnimationManager;

struct UpgradeItem {
    std::string iconName;       // 왼쪽 아이콘 이미지
    std::string titleText;      // 가운데 메이저 텍스트 (1줄)
    std::string descText;       // 가운데 아래 설명 텍스트 (2줄)
    std::string buttonImage;    // 오른쪽 버튼 이미지
    std::string buttonText;     // 오른쪽 버튼 텍스트
    
    UpgradeItem(const std::string& icon = "", const std::string& title = "",
                const std::string& desc = "", const std::string& btnImg = "",
                const std::string& btnTxt = "")
        : iconName(icon), titleText(title), descText(desc),
          buttonImage(btnImg), buttonText(btnTxt) {}
};

class UpgradeListWidget : public UIWidget {
private:
    SDL_Renderer* renderer;
    TextRenderer* textRenderer;
    std::vector<UpgradeItem> items;
    
    // 레이아웃
    int itemHeight;             // 각 항목 높이 (픽셀)
    int iconSize;               // 아이콘 크기 (픽셀)
    int iconMargin;             // 아이콘과 텍스트 간격
    int titleFontSize;          // 메이저 텍스트 폰트 크기
    int descFontSize;           // 설명 텍스트 폰트 크기
    int buttonWidth;            // 버튼 너비
    int buttonHeight;           // 버튼 높이
    int buttonFontSize;         // 버튼 텍스트 폰트 크기
    
    SDL_Color titleColor;
    SDL_Color descColor;
    SDL_Color buttonTextColor;
    
    std::string itemBackgroundImage;
    bool useItemBackgroundNinePatch;
    bool useItemBackgroundThreePatch;
    bool useButtonNinePatch;
    bool useButtonThreePatch;
    
    int scrollOffset;
    int maxVisibleItems;
    
    // 마우스 드래그
    bool isDragging;
    bool wasDraggableOnMouseDown;
    int lastMouseY;
    int dragStartY;
    
    // 가상화: 보이는 항목만
    struct VisibleItem {
        int itemIndex;
        std::string rowBgElementId;
        std::string rowBgTextureId;
        std::string iconElementId;
        std::unique_ptr<BackgroundTextWidget> titleWidget;
        std::unique_ptr<BackgroundTextWidget> descWidget;
        std::string buttonWidgetName;  // WidgetManager에 등록된 ButtonWidget 이름
    };
    std::vector<VisibleItem> visibleItems;
    
    std::function<void(int)> onItemButtonClick;
    std::string uiElementId;
    WidgetManager* widgetManager;
    AnimationManager* animationManager;
    
    void updateVisibleItems();
    void createItemElement(int index);
    void removeItemElement(int index);
    void updateItemPositions();
    void clearVisibleItems(bool removeChildWidgets = true);

public:
    UpgradeListWidget(UiManager* uiMgr, ResourceManager* resMgr,
                     SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                     int itemH, int iconSz, int iconMarg,
                     int titleSize, int descSize,
                     int btnW, int btnH, int btnFontSize,
                     SDL_Color titleClr, SDL_Color descClr, SDL_Color btnTxtClr,
                     const SDL_Rect& rect,
                     float scale = 1.0f, float rotation = 0.0f, float alpha = 1.0f,
                     bool visible = true, bool clickable = true);
    
    std::string getUIElementIdentifier() const override { return uiElementId; }
    
    ~UpgradeListWidget() override;
    
    void addItem(const UpgradeItem& item);
    void addItem(const std::string& iconName, const std::string& title,
                 const std::string& desc, const std::string& buttonImage,
                 const std::string& buttonText);
    void clearItems();
    int getItemCount() const { return static_cast<int>(items.size()); }
    
    void setItemHeight(int height);
    void setIconSize(int size);
    void setIconMargin(int margin);
    void setTitleFontSize(int size);
    void setDescFontSize(int size);
    void setButtonSize(int w, int h);
    void setButtonFontSize(int size);
    void setTitleColor(SDL_Color color);
    void setDescColor(SDL_Color color);
    void setButtonTextColor(SDL_Color color);
    
    void setItemBackgroundImage(const std::string& imageName, bool useNinePatch = false, bool useThreePatch = false);
    void setButtonNinePatch(bool use) { useButtonNinePatch = use; }
    void setButtonThreePatch(bool use) { useButtonThreePatch = use; }
    
    void setWidgetManager(WidgetManager* wm) { widgetManager = wm; }
    void setAnimationManager(AnimationManager* am) { animationManager = am; }
    
    void setOnItemButtonClick(std::function<void(int)> callback) { onItemButtonClick = callback; }
    
    void setScrollOffset(int offset);
    int getScrollOffset() const { return scrollOffset; }
    void scrollToTop();
    void scrollToBottom();
    
    void handleEvent(const SDL_Event& event) override;
    void update(float deltaTime) override;
};
