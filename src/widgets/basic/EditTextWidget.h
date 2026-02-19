#pragma once
#include "../UIWidget.h"
#include <string>
#include "../../utils/sdl_includes.h"

class TextRenderer;
class ImageRenderer;

class EditTextWidget : public UIWidget {
private:
    std::string text;              // 현재 텍스트 내용
    std::string placeholder;       // placeholder 텍스트
    int cursorPosition;            // 커서 위치 (문자 인덱스)
    bool hasFocus;                 // 포커스 여부
    bool showCursor;               // 커서 표시 여부
    float cursorBlinkTimer;        // 커서 깜빡임 타이머
    
    // 렌더링 관련
    SDL_Renderer* renderer;
    TextRenderer* textRenderer;
    SDL_Texture* textTexture;      // 현재 텍스트 텍스처
    std::string currentTextureName; // ResourceManager에 등록된 텍스처 이름
    
    // 설정
    int maxLength;                 // 최대 길이 (0이면 무제한)
    bool multiline;                // 여러 줄 입력 허용 여부
    SDL_Color textColor;           // 텍스트 색상
    SDL_Color backgroundColor;     // 배경 색상
    SDL_Color borderColor;         // 테두리 색상
    int fontSize;                  // 폰트 크기
    int padding;                   // 내부 여백
    
    // UIElement 영역
    SDL_Rect textRect;             // 텍스트가 그려질 영역
    std::string uiElementId;        // UIElement ID (UiManager가 생성한 자동 ID)
    
    void updateCursorBlink(float deltaTime);  // 커서 깜빡임 업데이트
    
    // UIWidget 오버라이드
    std::string getUIElementIdentifier() const override { return uiElementId; }
    
public:
    EditTextWidget(UiManager* uiMgr, ResourceManager* resMgr,
                   SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                   const std::string& placeholder = "", int size = 16, 
                   SDL_Color textColor = {255, 255, 255, 255},
                   SDL_Color bgColor = {50, 50, 50, 255},
                   SDL_Color borderColor = {100, 100, 100, 255},
                   int maxLen = 0, bool multiLine = false);
    
    virtual ~EditTextWidget();
    
    // 포커스 관련 (UIWidget 오버라이드)
    void setFocus(bool focus) override;
    bool canReceiveFocus() const override { return true; }
    
    // 이벤트 처리
    void handleEvent(const SDL_Event& event) override;
    void update(float deltaTime) override;
    void render(SDL_Renderer* renderer) override;
    
    // 텍스트 조작
    void setText(const std::string& newText);
    const std::string& getText() const { return text; }
    void insertText(const std::string& str);
    void deleteChar();              // 백스페이스 (커서 앞 문자 삭제)
    void deleteCharForward();       // Delete 키 (커서 뒤 문자 삭제)
    void moveCursor(int direction); // -1: 왼쪽, +1: 오른쪽
    void setCursorPosition(int pos);
    
    // 설정
    void setMaxLength(int len) { maxLength = len; }
    void setMultiline(bool multi) { multiline = multi; }
    void setTextColor(SDL_Color color);
    void setBackgroundColor(SDL_Color color);
    void setBorderColor(SDL_Color color);
    void setFontSize(int size);
    
    // UIElement ID 반환
    std::string getUIElementId() const { return uiElementId; }
    
    // 텍스처 업데이트 (WidgetManager에서 크기 설정 후 호출)
    void updateTexture();
};

