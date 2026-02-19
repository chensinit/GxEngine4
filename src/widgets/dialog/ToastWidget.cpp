#include "ToastWidget.h"
#include "../basic/TextWidget.h"
#include "../../ui/uiManager.h"
#include "../../rendering/TextRenderer.h"
#include "../../rendering/ImageRenderer.h"
#include "../../animation/AnimationManager.h"
#include "../../animation/Animator.h"

ToastWidget::ToastWidget(UiManager* uiMgr, ResourceManager* resMgr,
                         SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                         AnimationManager* animMgr,
                         const SDL_Rect& rect, int fontSize, SDL_Color textColor)
    : UIWidget(uiMgr, resMgr),
      uiElementId(""),
      textElementId(""),
      renderer(sdlRenderer),
      textRenderer(txtRenderer),
      animationManager(animMgr),
      fadeInDuration(600),
      fadeOutDuration(1000),
      displayDuration(3000),
      defaultFontSize(fontSize),
      defaultTextColor(textColor) {
    
    ImageRenderer imageRenderer(renderer, rect.w, rect.h);
    imageRenderer.drawColor(SDL_Color{0, 0, 0, 180});
    SDL_Texture* bgTexture = imageRenderer.getTexture();
    
    int texW, texH;
    Uint32 format;
    int access;
    SDL_QueryTexture(bgTexture, &format, &access, &texW, &texH);
    SDL_Texture* copiedTexture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_TARGET, texW, texH);
    if (copiedTexture) {
        SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer, copiedTexture);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, bgTexture, nullptr, nullptr);
        SDL_SetRenderTarget(renderer, oldTarget);
        
        std::string textureId = resourceManager->registerTexture(copiedTexture);
        
        UIElement bgElement;
        bgElement.name = "";
        bgElement.texture = copiedTexture;
        bgElement.textureId = textureId;
        bgElement.rect = rect;
        bgElement.visible = false;
        bgElement.clickable = false;
        bgElement.alpha = 0.0f;
        bgElement.scale = 1.0f;
        bgElement.rotation = 0.0f;
        
        uiElementId = uiManager->addUIAndGetId(bgElement);
        uiManager->setAlwaysOnTop(uiElementId, true);
    } else {
        UIElement bgElement;
        bgElement.name = "";
        bgElement.texture = nullptr;
        bgElement.rect = rect;
        bgElement.visible = false;
        bgElement.clickable = false;
        bgElement.alpha = 0.0f;
        bgElement.scale = 1.0f;
        bgElement.rotation = 0.0f;
        uiElementId = uiManager->addUIAndGetId(bgElement);
        uiManager->setAlwaysOnTop(uiElementId, true);
    }
}

ToastWidget::~ToastWidget() {
    if (animationManager && !uiElementId.empty()) {
        animationManager->remove(uiElementId);
    }
}

void ToastWidget::createTextWidget(const std::string& text, int fontSize, SDL_Color textColor, const SDL_Rect& rect) {
    SDL_Color whiteColor = {255, 255, 255, 255};
    
    if (!textWidget) {
        textWidget = std::make_unique<TextWidget>(
            uiManager, resourceManager, renderer, textRenderer,
            text, fontSize, whiteColor, 0, -1, "center",
            0, 0, rect.w, rect.h
        );
        textElementId = textWidget->getUIElementId();
        uiManager->setParent(textElementId, uiElementId);
        uiManager->setAlwaysOnTop(textElementId, true);
    } else {
        textWidget->setText(text);
        textWidget->setFontSize(fontSize);
        textWidget->setTextColor(whiteColor);
    }
    
    auto* textElement = uiManager->findElementByName(textElementId);
    if (textElement) {
        int textW = textElement->rect.w;
        int textH = textElement->rect.h;
        textElement->rect.x = (rect.w - textW) / 2;
        textElement->rect.y = (rect.h - textH) / 2;
    }
}

void ToastWidget::showToast(const std::string& text, int displayDurationMs) {
    auto* bgElement = uiManager->findElementByName(uiElementId);
    if (!bgElement) return;
    
    createTextWidget(text, defaultFontSize, SDL_Color{255, 255, 255, 255}, bgElement->rect);
    
    auto* textElement = uiManager->findElementByName(textElementId);
    if (!textElement) return;
    
    uiManager->setVisible(uiElementId, true);
    
    if (animationManager) {
        animationManager->remove(uiElementId);
        
        auto anim = std::make_shared<Animator>(uiElementId);
        anim->alpha(0.0f, 1.0f, fadeInDuration);
        anim->delay(displayDurationMs);
        anim->alpha(1.0f, 0.0f, fadeOutDuration)
            .callback([this]() {
                auto* bg = uiManager->findElementByName(uiElementId);
                if (bg) bg->visible = false;
            });
        
        animationManager->add(uiElementId, anim);
    }
}

void ToastWidget::hide() {
    if (animationManager && !uiElementId.empty()) {
        animationManager->remove(uiElementId);
    }
    
    auto* bgElement = uiManager->findElementByName(uiElementId);
    if (bgElement) {
        bgElement->visible = false;
        bgElement->alpha = 0.0f;
    }
}

bool ToastWidget::isVisible() const {
    auto* bgElement = uiManager->findElementByName(uiElementId);
    return bgElement && bgElement->visible;
}
