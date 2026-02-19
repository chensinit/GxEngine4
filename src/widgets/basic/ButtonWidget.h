#pragma once
#include "../UIWidget.h"  // 같은 폴더
#include <string>
#include <functional>
#include "../../utils/sdl_includes.h"

class TextRenderer;
class ImageRenderer;
class AnimationManager;

class ButtonWidget : public UIWidget {
public:
    enum State {
        NORMAL,
        PRESSED,
        DISABLED
    };
    
private:
    std::string normalImageName;   // 평소 이미지 이름
    std::string pressedImageName;  // 눌렸을 때 이미지 이름 (사용 안 함, 호환성 유지)
    bool useNinePatch;              // 9패치 사용 여부
    bool useThreePatch;             // 3패치 사용 여부 (가로 3등분, 중간만 늘림)
    
    // 텍스트 관련
    std::string buttonText;        // 버튼에 표시할 텍스트
    int fontSize;                   // 폰트 크기
    SDL_Color textColor;            // 텍스트 색상
    
    State state;
    std::function<void()> onClickCallback;
    
    // UIElement ID (자동 생성된 ID 저장)
    std::string uiElementId;
    
    // 아이콘: 별도 UIElement. 높이 기준 비율로 크기·여백 계산 (옵션 없음)
    std::string iconImageName;
    std::string iconElementId;
    int iconSrcW, iconSrcH;  // 원본 텍스처 비율 (updateIconElement에서 크기 계산용)
    int iconDisplayW, iconDisplayH;  // 실제 표시 크기 (텍스트 X 계산용)
    int iconPadding;         // 왼쪽 여백 (height 비율)
    int iconTextSpacing;     // 아이콘~텍스트 (height 비율)
    
    // 현재 텍스처 ID (해제용)
    std::string currentTextureId;
    
    // Renderer들
    SDL_Renderer* renderer;
    TextRenderer* textRenderer;
    AnimationManager* animationManager;
    
public:
    ButtonWidget(UiManager* uiMgr, ResourceManager* resMgr,
                 SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                 const std::string& normalImg, const std::string& pressedImg = "",
                 const SDL_Rect& rect = SDL_Rect{0, 0, 0, 0},
                 AnimationManager* animMgr = nullptr);
    ~ButtonWidget();

    // 위치/크기 설정
    void setRect(const SDL_Rect& rect);
    void setRect(int x, int y, int w, int h);
    
    void setOnClick(std::function<void()> callback);
    void setState(State newState);
    State getState() const { return state; }
    void setEnabled(bool enabled);
    
    // 텍스트 설정
    void setText(const std::string& text);
    const std::string& getText() const { return buttonText; }
    void setFontSize(int size);
    void setTextColor(SDL_Color color);
    
    // 이미지 설정 (useNinePatch 우선, 그다음 useThreePatch)
    void setNormalImage(const std::string& imageName, bool useNinePatchParam = false, bool useThreePatchParam = false);

    /** 왼쪽 아이콘 설정. imageName 비우면 아이콘 제거. 크기/여백은 버튼 높이 기준 비율로 자동 계산 */
    void setIcon(const std::string& imageName);

    // UIWidget 오버라이드
    std::string getUIElementIdentifier() const override { return uiElementId; }
    bool canReceiveClick() const override { return state != DISABLED; }
    void onClick() override;  // 클릭 완료 시 호출
    void handleEvent(const SDL_Event& event) override;  // 마우스 이벤트 처리
    
private:
    bool isMouseDown = false;  // 마우스 상태 추적용
    bool wasClickableOnMouseDown = false;  // mouse_down 시점에 클릭 가능했는지 (애니메이션으로 작아져도 클릭 처리)
    void updateTexture();       // 상태에 따라 UIElement의 texture 변경 (이미지 + 텍스트)
    void updateIconElement();   // 아이콘 UIElement 생성/위치 갱신 (setIcon, setRect에서 호출)
};

