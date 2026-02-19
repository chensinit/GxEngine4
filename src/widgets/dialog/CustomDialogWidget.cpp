#include "CustomDialogWidget.h"
#include "../../ui/uiManager.h"
#include "../../animation/AnimationManager.h"
#include "../../animation/Animator.h"
#include "../../utils/logger.h"
#include "../../utils/sdl_includes.h"

CustomDialogWidget::CustomDialogWidget(UiManager* uiMgr, ResourceManager* resMgr,
                                       SDL_Renderer* sdlRenderer, AnimationManager* animMgr,
                                       const std::string& widgetName, const std::string& imageName, const SDL_Rect& rect)
    : UIWidget(uiMgr, resMgr),
      isVisible(false),
      renderer(sdlRenderer),
      animationManager(animMgr) {
    
    // 배경 UIElement 생성 (위젯 이름을 UIElement 이름으로 사용)
    createBackgroundElement(widgetName, imageName, rect);
    
    // 초기에는 숨김 상태 (자식들도 함께 숨김)
    auto* bgElement = uiManager->findElementByName(bgElementId);
    if (bgElement) {
        bgElement->visible = false;
        bgElement->scale = 0.5f;  // 애니메이션 시작 크기
    }
}

CustomDialogWidget::~CustomDialogWidget() {
    // UIElement는 씬 전환 시 UiManager::clear()에서 처리됨
}

void CustomDialogWidget::createBackgroundElement(const std::string& widgetName, const std::string& imageName, const SDL_Rect& rect) {
    // UIElement 이름을 위젯 이름과 동일하게 설정 (부모-자식 관계를 위해)
    bgElementId = widgetName;
    uiElementId = bgElementId;
    
    // 배경 이미지 설정
    std::string bgImageName = imageName;
    SDL_Texture* bgTexture = nullptr;
    
    if (!imageName.empty()) {
        // 이미지가 지정된 경우
        bgTexture = resourceManager->getTexture(imageName);
        if (!bgTexture) {
            Log::error("[CustomDialogWidget] Failed to load background texture: ", imageName);
        }
    }
    // 이미지가 없으면 투명 배경 (texture는 nullptr)
    
    // UIElement 생성
    UIElement bgElement;
    bgElement.name = bgElementId;
    bgElement.texture = bgTexture;
    bgElement.textureId = bgImageName;
    bgElement.rect = rect;
    bgElement.scale = 0.5f;  // 초기 크기 (애니메이션 시작 크기)
    bgElement.rotation = 0.0f;
    bgElement.alpha = 1.0f;
    bgElement.visible = false;  // 초기에는 숨김
    bgElement.clickable = false;  // 다이얼로그 배경은 클릭 불가
    
    uiManager->addUI(bgElement);
}

void CustomDialogWidget::show() {
    if (isVisible) return;
    
    isVisible = true;
    auto* bgElement = uiManager->findElementByName(bgElementId);
    if (!bgElement) return;
    
    // 부모만 표시 (자식들은 부모의 visible 상태를 자동으로 따름)
    bgElement->visible = true;
    
    // 애니메이션: 50% -> 120% -> 100%
    if (animationManager) {
        bgElement->scale = 0.5f;  // 애니메이션 시작 크기
        auto anim = std::make_shared<Animator>(bgElementId);
        anim->scale(0.5f, 1.2f, 200)  // 50% -> 120% (200ms)
            .scale(1.2f, 1.0f, 100);  // 120% -> 100% (100ms)
        animationManager->add(bgElementId, anim);
    } else {
        // AnimationManager가 없으면 즉시 표시
        bgElement->scale = 1.0f;
    }
}

void CustomDialogWidget::hide() {
    if (!isVisible) return;
    
    isVisible = false;
    auto* bgElement = uiManager->findElementByName(bgElementId);
    if (!bgElement) return;
    
    // 애니메이션: 100% -> 50%
    if (animationManager) {
        auto anim = std::make_shared<Animator>(bgElementId);
        anim->scale(1.0f, 0.5f, 200)  // 100% -> 50% (200ms)
            .callback([this]() {
                // 애니메이션 완료 후 숨김
                auto* bgElement = uiManager->findElementByName(bgElementId);
                if (bgElement) {
                    bgElement->visible = false;
                }
            });
        animationManager->add(bgElementId, anim);
    } else {
        // AnimationManager가 없으면 즉시 숨김
        bgElement->visible = false;
        bgElement->scale = 0.5f;
    }
}

void CustomDialogWidget::dismiss() {
    hide();
}

