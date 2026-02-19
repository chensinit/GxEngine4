#pragma once

#include "../UIWidget.h"
#include <string>
#include <vector>
#include <map>
#include "../../utils/sdl_includes.h"

class TextRenderer;

class SectionGridWidget : public UIWidget {
private:
    enum class ItemKind {
        Header,
        Card
    };

    struct OverlayDef {
        std::string imageName;
        int height = 36;
    };

    struct Item {
        ItemKind kind;
        std::string headerImage;
        std::string cardImage;
        std::string overlayId;  // empty = no overlay
    };

    struct ItemLayout {
        int y = 0;
        int height = 0;
        int headerW = 0, headerH = 0, headerX = 0;
        std::string headerImage;
        int baseX = 0, baseY = 0, frameW = 0, frameH = 0;  // for cards: cell rect for image
        int imgX = 0, imgY = 0, imgW = 0, imgH = 0;  // computed when creating visible card
        int col = 0;
    };

    struct CreatedElement {
        ItemKind kind;
        std::string bgElementId;
        std::string imageElementId;
        std::string overlayElementId;
        std::string textElementId;
    };

    std::map<std::string, OverlayDef> overlays;
    std::vector<Item> items;
    std::vector<ItemLayout> layoutData;
    std::map<size_t, CreatedElement> createdElements;

    int columns;
    int cellWidth;
    int cellHeight;
    int cellMargin;
    int headerMarginV;
    float headerImageScale = 0.5f;

    std::string uiElementId;
    std::string contentElementId;

    int scrollOffset = 0;
    int totalContentHeight = 0;

    bool isDragging = false;
    int lastMouseY = 0;
    int dragStartY = 0;

    SDL_Renderer* renderer = nullptr;
    TextRenderer* textRenderer = nullptr;

    void clearElements();
    void computeLayout(int maxWidth);
    void ensureContentContainer(int maxWidth);
    void updateVisibleItems();
    void setScrollOffset(int offset);

public:
    SectionGridWidget(UiManager* uiMgr,
                      ResourceManager* resMgr,
                      SDL_Renderer* sdlRenderer,
                      TextRenderer* txtRenderer,
                      int cols,
                      int cellW,
                      int cellH,
                      const SDL_Rect& rect,
                      int cellMarg = 8,
                      int headerMargV = 8,
                      float scale = 1.0f,
                      float rotation = 0.0f,
                      float alpha = 1.0f,
                      bool visible = true,
                      bool clickable = true);

    ~SectionGridWidget() override;

    std::string getUIElementIdentifier() const override { return uiElementId; }

    void clear();
    void addOverlay(const std::string& overlayId, const std::string& imageName, int height = 36);
    void addHeader(const std::string& imageName);
    void addCard(const std::string& imageName, const std::string& overlayId = "");

    void layout();

    void setColumns(int cols);
    void setCellSize(int w, int h);
    void setCellMargin(int margin);
    void setHeaderMarginV(int marginV);
    void setHeaderImageScale(float scale);

    void update(float deltaTime) override;
    void handleEvent(const SDL_Event& event) override;
};
