#pragma once
#include "../UIWidget.h"
#include <string>
#include "../../utils/sdl_includes.h"

class ImageRenderer;
class AnimationManager;

enum class BackgroundImageMode {
    STRETCH,   // 확대해서 채우기
    TILE,      // 바둑판 배열
    FIT,       // 스케일에 맞게 넣고 남는 공간은 비워두기
    SCROLL_H,  // height에 맞춰 늘리고, 가로로 스크롤 (child + Animator)
    SCROLL_V   // width에 맞춰 늘리고, 세로로 스크롤 (child + Animator)
};

class BackgroundWidget : public UIWidget {
private:
    SDL_Renderer* renderer;
    AnimationManager* animationManager = nullptr;

    // 색상 배경
    SDL_Color backgroundColor;
    bool useColor;

    // 이미지 배경
    std::string imageName;
    bool useImage;
    BackgroundImageMode imageMode;

    // UIElement ID
    std::string uiElementId;       // parent(container) ID
    std::string scrollChildId;     // scroll 모드 시 child 이미지 ID

    // 현재 텍스처 ID (해제용)
    std::string currentTextureId;

    SDL_Rect rect;

    float scrollDuration = 0.f;    // scroll 모드용 duration(초)

    void updateTexture();
    void createScrollChild();      // scroll 모드: child 이미지 요소 생성
    void setupScrollAnimator();    // scroll 모드: Animator 등록

public:
    BackgroundWidget(UiManager* uiMgr, ResourceManager* resMgr,
                    SDL_Renderer* sdlRenderer,
                    const SDL_Rect& rect,
                    AnimationManager* animMgr = nullptr);

    void setAnimationManager(AnimationManager* am) { animationManager = am; }

    void setBackgroundColor(SDL_Color color);
    void setBackgroundImage(const std::string& imageName, BackgroundImageMode mode = BackgroundImageMode::STRETCH);
    void setScrollDuration(float seconds);

    void setRect(const SDL_Rect& newRect);
    void setRect(int x, int y, int w, int h);

    std::string getUIElementIdentifier() const override { return uiElementId; }
    void update(float deltaTime) override;
};
