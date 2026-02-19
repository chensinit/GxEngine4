#pragma once
#include "../UIWidget.h"
#include <string>
#include <vector>
#include <memory>
#include "../../utils/sdl_includes.h"

class TextRenderer;
class ImageRenderer;
class BackgroundTextWidget;

enum class ChatAlignment {
    LEFT,   // 왼쪽 정렬 (아이콘 왼쪽)
    RIGHT   // 오른쪽 정렬 (아이콘 오른쪽)
};

struct ChatMessage {
    std::string text;           // 메시지 텍스트
    std::string iconName;        // 아이콘 이미지 이름 (빈 문자열이면 아이콘 없음)
    ChatAlignment alignment;     // 정렬 방향
    
    ChatMessage(const std::string& t, const std::string& icon = "", ChatAlignment align = ChatAlignment::LEFT)
        : text(t), iconName(icon), alignment(align) {}
};

class ChatListWidget : public UIWidget {
private:
    SDL_Renderer* renderer;
    TextRenderer* textRenderer;
    std::vector<ChatMessage> messages;  // 채팅 메시지들
    int itemHeight;                     // 각 항목의 높이 (픽셀)
    int fontSize;                       // 폰트 크기
    SDL_Color textColor;                // 텍스트 색상
    std::string backgroundImageName;    // 배경 이미지 이름 (텍스트용)
    bool useNinePatch;                  // 9패치 사용 여부
    bool useThreePatch;                 // 3패치 사용 여부
    int iconSize;                       // 아이콘 크기 (픽셀)
    int iconTextSpacing;                // 아이콘과 텍스트 간격 (픽셀)
    int scrollOffset;                   // 스크롤 오프셋 (픽셀)
    int maxVisibleItems;                // 화면에 보이는 최대 항목 수
    
    // 마우스 드래그 관련
    bool isDragging;
    bool wasDraggableOnMouseDown = false;  // mouse_down 시점에 드래그 가능했는지 (범위 벗어나도 mouse_up 처리)
    int lastMouseY;
    int dragStartY;
    
    // 각 메시지의 BackgroundTextWidget과 아이콘 UIElement 이름 목록
    std::vector<std::unique_ptr<BackgroundTextWidget>> messageTextWidgets;  // 텍스트 위젯들
    std::vector<std::string> messageIconElementNames;  // 아이콘 UIElement 이름들
    
    void updateMessageElements();  // 메시지 UIElement 업데이트
    void clearMessageElements();    // 메시지 UIElement 제거
    void updateMessagePositions();  // 메시지 UIElement 위치 업데이트 (스크롤 반영)
    
    std::string uiElementId;  // UIElement ID
    
public:
    ChatListWidget(UiManager* uiMgr, ResourceManager* resMgr,
                   SDL_Renderer* renderer, TextRenderer* textRenderer,
                   int itemH, int size, SDL_Color color, int iconSz = 40, int spacing = 10,
                   const SDL_Rect& rect = SDL_Rect{0, 0, 0, 0},
                   float scale = 1.0f, float rotation = 0.0f, float alpha = 1.0f,
                   bool visible = true, bool clickable = true);
    
    std::string getUIElementIdentifier() const override { return uiElementId; }
    
    virtual ~ChatListWidget();
    
    // 리스트 조작
    void setMessages(const std::vector<ChatMessage>& newMessages);
    void addMessage(const std::string& text, const std::string& iconName = "", ChatAlignment alignment = ChatAlignment::LEFT);
    void clearMessages();
    int getMessageCount() const { return static_cast<int>(messages.size()); }
    
    // 스타일 설정
    void setItemHeight(int height);
    void setFontSize(int size);
    void setTextColor(SDL_Color color);
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

