#pragma once

#include "../UIWidget.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <set>
#include "../../utils/sdl_includes.h"

class TextRenderer;

// 텍스트 정렬 (가로)
enum class BannerTextAlignH { Left = 0, Center = 1, Right = 2 };
// 텍스트 정렬 (세로)
enum class BannerTextAlignV { Top = 0, Center = 1, Bottom = 2 };

struct BannerItem {
    std::string title;
    std::string imageName;  // 항목별 배경 이미지 (비어있으면 전역 배경 사용)
};

class BannerListWidget : public UIWidget {
private:
    static const int TEXT_PADDING = 8;

    SDL_Renderer* renderer;
    TextRenderer* textRenderer;
    std::vector<BannerItem> items;

    int itemHeight;
    int itemMargin;  // child 간 세로 간격 (px)
    std::string backgroundImageName;
    SDL_Color backgroundColor;
    int fontSize;
    SDL_Color textColor;
    BannerTextAlignH textAlignH;
    BannerTextAlignV textAlignV;

    int scrollOffset;
    int maxVisibleItems;

    bool isDragging;
    bool wasDraggableOnMouseDown;
    int lastMouseY;
    int dragStartY;

    struct VisibleItem {
        int itemIndex;
        std::string bgElementId;
        std::string bgTextureId;
        std::string textElementId;
        std::string textTextureId;
    };
    std::vector<VisibleItem> visibleItems;

    std::function<void(int)> onItemClick;
    std::string uiElementId;

    void updateVisibleItems();
    void createItemElement(int index);
    void updateItemPositions();
    void clearVisibleItems();
    SDL_Texture* createBackgroundTexture(int w, int h, const std::string& imageName,
                                         std::string& outTextureId);
    void computeTextPosition(int itemW, int itemH, int textW, int textH,
                            int& outX, int& outY);

public:
    BannerListWidget(UiManager* uiMgr, ResourceManager* resMgr,
                    SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                    int itemH, const std::string& bgImage, SDL_Color bgColor,
                    int fontSize, SDL_Color txtColor,
                    BannerTextAlignH alignH, BannerTextAlignV alignV,
                    const SDL_Rect& rect,
                    float scale = 1.0f, float rotation = 0.0f, float alpha = 1.0f,
                    bool visible = true, bool clickable = true);

    std::string getUIElementIdentifier() const override { return uiElementId; }

    ~BannerListWidget() override;

    void addItem(const std::string& title);
    void addItem(const std::string& title, const std::string& imageName);
    void clearItems();
    int getItemCount() const { return static_cast<int>(items.size()); }

    void setItemHeight(int height);
    void setItemMargin(int margin);
    void setBackgroundImage(const std::string& imageName);
    void setBackgroundColor(SDL_Color color);
    void setFontSize(int size);
    void setTextColor(SDL_Color color);
    void setTextAlign(BannerTextAlignH h, BannerTextAlignV v);

    void setOnItemClick(std::function<void(int)> callback) { onItemClick = callback; }

    void setScrollOffset(int offset);
    int getScrollOffset() const { return scrollOffset; }
    void scrollToTop();
    void scrollToBottom();

    void handleEvent(const SDL_Event& event) override;
    void update(float deltaTime) override;
};
