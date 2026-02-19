#pragma once
#include "../UIWidget.h"
#include <string>
#include <vector>
#include "../../utils/sdl_includes.h"

class TextRenderer;

class TextListWidget : public UIWidget {
private:
    SDL_Renderer* renderer;
    TextRenderer* textRenderer;
    std::vector<std::string> items;          // 텍스트 리스트 항목들
    int itemHeight;                          // 각 항목의 높이 (픽셀)
    int fontSize;                             // 폰트 크기
    SDL_Color textColor;                      // 텍스트 색상
    int scrollOffset;                         // 스크롤 오프셋 (픽셀)
    int maxVisibleItems;                      // 화면에 보이는 최대 항목 수
    
    // 마우스 드래그 관련
    bool isDragging;
    bool wasDraggableOnMouseDown = false;  // mouse_down 시점에 드래그 가능했는지 (범위 벗어나도 mouse_up 처리)
    int lastMouseY;
    int dragStartY;
    
    // 각 항목의 텍스처 캐시 (성능 최적화)
    struct ItemTexture {
        std::string text;
        SDL_Texture* texture;
        std::string textureId;  // ResourceManager 텍스처 ID
        int width;
        int height;
    };
    std::vector<ItemTexture> itemTextures;
    
    void updateItemTextures();  // 텍스처 업데이트
    void clearItemTextures();    // 텍스처 해제
    
    std::string uiElementId;  // UIElement ID
    
public:
    TextListWidget(UiManager* uiMgr, ResourceManager* resMgr,
                 SDL_Renderer* renderer, TextRenderer* textRenderer,
                 int itemH, int size, SDL_Color color,
                 const SDL_Rect& rect = SDL_Rect{0, 0, 0, 0},
                 float scale = 1.0f, float rotation = 0.0f, float alpha = 1.0f,
                 bool visible = true, bool clickable = true);
    
    std::string getUIElementIdentifier() const override { return uiElementId; }
    
    virtual ~TextListWidget();
    
    // 리스트 조작
    void setItems(const std::vector<std::string>& newItems);
    void addItem(const std::string& item);
    void clearItems();
    int getItemCount() const { return static_cast<int>(items.size()); }
    
    // 스타일 설정
    void setItemHeight(int height);
    void setFontSize(int size);
    void setTextColor(SDL_Color color);
    
    // 스크롤 조작
    void setScrollOffset(int offset);
    int getScrollOffset() const { return scrollOffset; }
    void scrollToTop();
    void scrollToBottom();
    
    // 이벤트 처리
    void handleEvent(const SDL_Event& event) override;
    
    // 렌더링
    void render(SDL_Renderer* renderer) override;
};

