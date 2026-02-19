#pragma once
#include "../UIWidget.h"
#include <string>
#include <vector>
#include <memory>
#include <set>
#include "../../utils/sdl_includes.h"

class TextRenderer;
class BackgroundTextWidget;

enum class ListItemType {
    CENTER_TEXT,        // 가운데 정렬된 text
    LEFT_ICON_TEXT,     // 왼쪽 아이콘 + text (채팅용)
    RIGHT_ICON_TEXT,    // 오른쪽 아이콘 + text (채팅용)
    CENTER_IMAGE        // 가운데 이미지
};

struct ListItem {
    ListItemType type;
    std::string text;           // 텍스트 (CENTER_TEXT, LEFT_ICON_TEXT, RIGHT_ICON_TEXT)
    std::string iconName;       // 아이콘 이미지 이름 (LEFT_ICON_TEXT, RIGHT_ICON_TEXT)
    std::string imageName;      // 이미지 이름 (CENTER_IMAGE)
    
    ListItem(ListItemType t, const std::string& txt = "", 
             const std::string& icon = "", const std::string& img = "")
        : type(t), text(txt), iconName(icon), imageName(img) {}
};

class MultiTypeListWidget : public UIWidget {
private:
    SDL_Renderer* renderer;
    TextRenderer* textRenderer;
    std::vector<ListItem> items;          // 리스트 항목들
    int itemHeight;                       // 각 항목의 높이 (픽셀, 고정)
    int fontSize;                         // 폰트 크기
    SDL_Color textColor;                  // 텍스트 색상
    SDL_Color backgroundColor;            // 배경 색상 (텍스트용, 배경 이미지가 없을 때)
    std::string backgroundImageName;      // 배경 이미지 이름 (텍스트용)
    bool useNinePatch;                    // 9패치 사용 여부
    bool useThreePatch;                   // 3패치 사용 여부
    int iconSize;                         // 아이콘 크기 (픽셀)
    int iconTextSpacing;                  // 아이콘과 텍스트 간격 (픽셀)
    int scrollOffset;                      // 스크롤 오프셋 (픽셀)
    int maxVisibleItems;                  // 화면에 보이는 최대 항목 수
    
    // 마우스 드래그 관련
    bool isDragging;
    bool wasDraggableOnMouseDown = false;  // mouse_down 시점에 드래그 가능했는지 (범위 벗어나도 mouse_up 처리)
    int lastMouseY;
    int dragStartY;
    
    // 보이는 항목만 저장 (가상화)
    struct VisibleItem {
        int itemIndex;                                    // 원본 items의 인덱스
        std::unique_ptr<BackgroundTextWidget> textWidget; // 텍스트 위젯 (있는 경우)
        std::string iconElementId;                        // 아이콘 UIElement ID (있는 경우)
        std::string imageElementId;                      // 이미지 UIElement ID (있는 경우)
    };
    std::vector<VisibleItem> visibleItems;  // 보이는 항목만
    
    void updateVisibleItems();      // 보이는 항목만 생성/제거
    void createItemElement(int index);  // 특정 인덱스의 아이템 UIElement 생성
    void removeItemElement(int index);  // 특정 인덱스의 아이템 UIElement 제거
    void updateItemPositions();     // 보이는 항목들의 위치 업데이트 (스크롤 반영)
    void clearVisibleItems();       // 모든 보이는 항목 제거
    
    std::string uiElementId;  // 리스트 컨테이너 UIElement ID
    
public:
    MultiTypeListWidget(UiManager* uiMgr, ResourceManager* resMgr,
                       SDL_Renderer* renderer, TextRenderer* textRenderer,
                       int itemH, int size, SDL_Color txtColor, SDL_Color bgColor = {255, 255, 255, 255},
                       int iconSz = 40, int spacing = 10,
                       const SDL_Rect& rect = SDL_Rect{0, 0, 0, 0},
                       float scale = 1.0f, float rotation = 0.0f, float alpha = 1.0f,
                       bool visible = true, bool clickable = true);
    
    std::string getUIElementIdentifier() const override { return uiElementId; }
    
    virtual ~MultiTypeListWidget();
    
    // 리스트 조작
    void setItems(const std::vector<ListItem>& newItems);
    void addItem(const ListItem& item);
    void addItem(ListItemType type, const std::string& text = "", 
                 const std::string& iconName = "", const std::string& imageName = "");
    void clearItems();
    int getItemCount() const { return static_cast<int>(items.size()); }
    
    // 스타일 설정
    void setItemHeight(int height);
    void setFontSize(int size);
    void setTextColor(SDL_Color color);
    void setBackgroundColor(SDL_Color color);
    void setBackgroundImage(const std::string& imageName, bool useNinePatch = false, bool useThreePatch = false);  // 배경 이미지 설정
    void setIconSize(int size);
    void setIconTextSpacing(int spacing);
    
    // 스크롤 조작
    void setScrollOffset(int offset);
    int getScrollOffset() const { return scrollOffset; }
    void scrollToTop();
    void scrollToBottom();
    
    // 이벤트 처리
    void handleEvent(const SDL_Event& event) override;
    
    // 업데이트 (스크롤 위치 조정)
    void update(float deltaTime) override;
};

