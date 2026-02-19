#pragma once
#include "../UIWidget.h"
#include <string>
#include "../../utils/sdl_includes.h"

class TextRenderer;
class ImageRenderer;

class BackgroundTextWidget : public UIWidget {
private:
    std::string text;              // 현재 텍스트 내용
    int fontSize;                   // 폰트 크기
    SDL_Color textColor;            // 텍스트 색상
    SDL_Color backgroundColor;      // 배경 색상
    std::string backgroundImageName; // 배경 이미지 이름 (빈 문자열이면 배경색 사용)
    bool useNinePatch;                // 9패치 사용 여부
    bool useThreePatch;               // 3패치 사용 여부 (가로 3등분)
    
    std::string textElementId;      // 텍스트 UIElement ID
    std::string backgroundElementId; // 배경 UIElement ID
    
    SDL_Texture* textTexture;       // 텍스트 텍스처
    SDL_Texture* backgroundTexture; // 배경 텍스처
    std::string textTextureId;      // 텍스트 텍스처 ID
    std::string backgroundTextureId; // 배경 텍스처 ID
    
    // 텍스트 영역 크기 (wrapWidth, maxHeight)
    int wrapWidth;
    int maxHeight;

    std::string textAlign;  // "left", "center", "right" - 배경 내 텍스트 정렬

    int fixedBackgroundWidth;   // > 0이면 배경 고정 너비 (우측정렬 등에서 필요)
    int fixedBackgroundHeight;  // > 0이면 배경 고정 높이
    
    void updateTextures();  // 텍스트/배경 변경 시 텍스처 재생성
    
    // UIWidget 오버라이드
    std::string getUIElementIdentifier() const override { return backgroundElementId; }
    
    SDL_Renderer* renderer;
    TextRenderer* textRenderer;
    
public:
    BackgroundTextWidget(UiManager* uiMgr, ResourceManager* resMgr,
                        SDL_Renderer* renderer,
                        TextRenderer* textRenderer,
                        const std::string& initialText, int size, 
                        SDL_Color textColor, SDL_Color bgColor,
                        int wrapW = 0, int maxH = -1);
    
    virtual ~BackgroundTextWidget();
    
    // 텍스트 조작
    void setText(const std::string& newText);
    const std::string& getText() const { return text; }
    
    // 스타일 변경
    void setFontSize(int size);
    void setTextColor(SDL_Color color);
    void setBackgroundColor(SDL_Color color);
    void setBackgroundImage(const std::string& imageName, bool useNinePatch = false, bool useThreePatch = false);  // 배경 이미지 설정
    void setFixedBackgroundSize(int w, int h);  // 배경 고정 크기 (0,0이면 텍스트 길이에 맞춤)
    void setTextAlign(const std::string& align);  // "left", "center", "right"
    void setWrapWidth(int width);
    void setMaxHeight(int height);
    
    int getFontSize() const { return fontSize; }
    SDL_Color getTextColor() const { return textColor; }
    SDL_Color getBackgroundColor() const { return backgroundColor; }
    
    // UIElement ID 반환
    std::string getBackgroundElementId() const { return backgroundElementId; }
    std::string getTextElementId() const { return textElementId; }
};

