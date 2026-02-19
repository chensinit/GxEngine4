#pragma once
#include "../UIWidget.h"
#include <string>
#include <vector>
#include "../../utils/sdl_includes.h"

class VerticalGridWidget : public UIWidget {
private:
    std::vector<std::string> items;   // 이미지 이름들
    int columns;                       // 열 개수
    int cellWidth;                     // 셀 너비 (픽셀)
    int cellHeight;                    // 셀 높이 (픽셀)
    int cellMargin;                    // 셀 간 간격 (프레임 크기 자동 감소)
    int cellMarginH;                   // 가로 마진 (0이면 cellMargin 사용)
    int scrollOffset;                  // 스크롤 오프셋 (픽셀)
    std::string cellBackgroundImage;   // 셀 배경 이미지 (프레임, 비우면 없음)
    
    // 가상화: 보이는 셀만 UIElement로 생성
    struct VisibleCell {
        int itemIndex;
        std::string frameElementId;    // 셀 배경(프레임) UIElement ID
        std::string imageElementId;    // 아이템 이미지 UIElement ID
    };
    std::vector<VisibleCell> visibleCells;
    
    // 마우스 드래그 스크롤
    bool isDragging = false;
    int lastMouseY = 0;
    int dragStartY = 0;
    
    std::string uiElementId;
    
    void updateVisibleCells();
    void createCellElement(int index);
    void removeCellElement(int index);
    void updateCellPositions();
    void clearVisibleCells();

public:
    VerticalGridWidget(UiManager* uiMgr, ResourceManager* resMgr,
                      int cols, int cellW, int cellH,
                      const SDL_Rect& rect,
                      const std::string& cellBgImage = "",
                      int cellMarg = 8,
                      float scale = 1.0f, float rotation = 0.0f, float alpha = 1.0f,
                      bool visible = true, bool clickable = true);
    
    std::string getUIElementIdentifier() const override { return uiElementId; }
    
    ~VerticalGridWidget() override;
    
    void addItem(const std::string& imageName);
    void clearItems();
    int getItemCount() const { return static_cast<int>(items.size()); }
    
    void setColumns(int cols);
    void setCellSize(int w, int h);
    void setCellMargin(int margin);
    void setCellMarginH(int marginH);
    void setCellBackgroundImage(const std::string& imageName);
    
    void setScrollOffset(int offset);
    void scrollToTop();
    void scrollToBottom();
    
    void handleEvent(const SDL_Event& event) override;
    void update(float deltaTime) override;
};
