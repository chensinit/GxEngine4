#pragma once
#include "../UIWidget.h"
#include <string>
#include "../../utils/sdl_includes.h"

class TextRenderer;

class TextWidget : public UIWidget {
private:
    std::string text;              // 현재 텍스트 내용
    int fontSize;                   // 폰트 크기
    SDL_Color textColor;            // 텍스트 색상
    SDL_Texture* currentTexture;    // 현재 텍스처 포인터 (ResourceManager에서 관리)
    std::string currentTextureName; // 현재 텍스처 이름 (ResourceManager 해제용)
    std::string uiElementId;        // UIElement ID (UiManager가 생성한 자동 ID)
    
    // 텍스트 영역 크기 (wrapWidth, maxHeight)
    int wrapWidth;
    int maxHeight;
    
    // 정렬 및 컨테이너 영역 (textAlign "center" 시 컨테이너 내 가로 가운데)
    std::string textAlign;
    SDL_Rect containerRect;  // x,y,w,h. w<0 이면 미사용
    void updateTexture();  // 텍스트 변경 시 텍스처 재생성
    
    // UIWidget 오버라이드
    std::string getUIElementIdentifier() const override { return uiElementId; }
    
    SDL_Renderer* renderer;
    TextRenderer* textRenderer;
    
public:
    TextWidget(UiManager* uiMgr, ResourceManager* resMgr,
              SDL_Renderer* renderer,
              TextRenderer* textRenderer,
              const std::string& initialText, int size, SDL_Color color,
              int wrapW = 0, int maxH = -1,
              const std::string& align = "", int contX = -1, int contY = -1, int contW = -1, int contH = -1);
    
    virtual ~TextWidget();
    
    // 텍스트 조작
    void setText(const std::string& newText);
    const std::string& getText() const { return text; }
    
    // 스타일 변경
    void setFontSize(int size);
    void setTextColor(SDL_Color color);
    void setWrapWidth(int width);
    void setMaxHeight(int height);
    
    int getFontSize() const { return fontSize; }
    SDL_Color getTextColor() const { return textColor; }
    
    // UIElement ID 반환
    std::string getUIElementId() const { return uiElementId; }
};

