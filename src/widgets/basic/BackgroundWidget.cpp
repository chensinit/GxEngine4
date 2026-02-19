#include "BackgroundWidget.h"
#include "../../animation/Animator.h"
#include "../../animation/AnimationManager.h"
#include "../../ui/uiManager.h"
#include "../../rendering/ImageRenderer.h"
#include "../../resource/resourceManager.h"
#include "../../utils/logger.h"
#include <algorithm>

BackgroundWidget::BackgroundWidget(UiManager* uiMgr, ResourceManager* resMgr,
                                   SDL_Renderer* sdlRenderer,
                                   const SDL_Rect& rect,
                                   AnimationManager* animMgr)
    : UIWidget(uiMgr, resMgr),
      renderer(sdlRenderer),
      animationManager(animMgr),
      backgroundColor{0, 0, 0, 255},
      useColor(false),
      useImage(false),
      imageMode(BackgroundImageMode::STRETCH),
      rect(rect) {

    // parent(container) UIElement - 클리핑용, texture는 scroll 모드에서 nullptr
    UIElement element;
    element.name = "";
    element.texture = nullptr;
    element.rect = rect;
    element.visible = true;
    element.clickable = false;
    element.scale = 1.0f;
    element.rotation = 0.0f;
    element.alpha = 1.0f;
    uiElementId = uiManager->addUIAndGetId(element);
}

void BackgroundWidget::setBackgroundColor(SDL_Color color) {
    backgroundColor = color;
    useColor = true;
    useImage = false;
    scrollChildId.clear();
    updateTexture();
}

void BackgroundWidget::setBackgroundImage(const std::string& imgName, BackgroundImageMode mode) {
    imageName = imgName;
    useImage = true;
    useColor = false;
    imageMode = mode;
    scrollChildId.clear();
    updateTexture();
}

void BackgroundWidget::setScrollDuration(float seconds) {
    scrollDuration = std::max(0.001f, seconds);
    setupScrollAnimator();
}

void BackgroundWidget::setRect(const SDL_Rect& newRect) {
    rect = newRect;
    updateTexture();
}

void BackgroundWidget::setRect(int x, int y, int w, int h) {
    rect = {x, y, w, h};
    updateTexture();
}

void BackgroundWidget::createScrollChild() {
    if (!animationManager || imageName.empty()) return;

    SDL_Texture* baseTexture = resourceManager->getTexture(imageName);
    if (!baseTexture) return;

    int imgW, imgH;
    SDL_QueryTexture(baseTexture, nullptr, nullptr, &imgW, &imgH);
    if (imgW <= 0 || imgH <= 0) return;

    const int w = rect.w, h = rect.h;
    if (w <= 0 || h <= 0) return;

    int childW, childH;
    if (imageMode == BackgroundImageMode::SCROLL_H) {
        float scale = static_cast<float>(h) / imgH;
        childW = static_cast<int>(imgW * scale);
        childH = h;
    } else {
        float scale = static_cast<float>(w) / imgW;
        childW = w;
        childH = static_cast<int>(imgH * scale);
    }

    UIElement childEl;
    childEl.name = "";
    childEl.texture = baseTexture;
    childEl.textureId = imageName;
    childEl.rect = {0, 0, childW, childH};
    childEl.visible = true;
    childEl.clickable = false;
    childEl.scale = 1.0f;
    childEl.rotation = 0.0f;
    childEl.alpha = 1.0f;

    scrollChildId = uiManager->addUIAndGetId(childEl);
    uiManager->setParent(scrollChildId, uiElementId);
}

void BackgroundWidget::setupScrollAnimator() {
    if (!animationManager || scrollChildId.empty() || scrollDuration <= 0.f) return;

    animationManager->remove(scrollChildId);

    auto* childEl = uiManager->findElementByName(scrollChildId);
    if (!childEl) return;

    const int w = rect.w, h = rect.h;
    int childW = childEl->rect.w;
    int childH = childEl->rect.h;

    int endX, endY;
    if (imageMode == BackgroundImageMode::SCROLL_H) {
        endX = -std::max(0, childW - w);
        endY = 0;
    } else {
        endX = 0;
        endY = -std::max(0, childH - h);
    }

    int durationMs = static_cast<int>(scrollDuration * 1000.f);
    auto anim = std::make_shared<Animator>(scrollChildId);
    anim->moveTo(0, 0, endX, endY, durationMs);   // 한쪽 방향
    anim->moveTo(endX, endY, 0, 0, durationMs);   // 반대 방향으로 복귀
    anim->repeat(-1);
    animationManager->add(scrollChildId, anim);
}

void BackgroundWidget::update(float deltaTime) {
    (void)deltaTime;
    // scroll은 Animator가 처리
}

void BackgroundWidget::updateTexture() {
    if (!currentTextureId.empty()) {
        resourceManager->unregisterTexture(currentTextureId);
        currentTextureId = "";
    }

    if (!scrollChildId.empty()) {
        animationManager->remove(scrollChildId);
        uiManager->removeUI(scrollChildId);
        scrollChildId.clear();
    }

    SDL_Texture* texture = nullptr;
    std::string textureId = "";
    int width = rect.w;
    int height = rect.h;

    if (width <= 0 || height <= 0) {
        Log::error("[BackgroundWidget] Invalid rect size: ", width, "x", height);
        return;
    }

    if (useColor) {
        ImageRenderer imageRenderer(renderer, width, height);
        imageRenderer.drawColor(backgroundColor);
        SDL_Texture* baseTexture = imageRenderer.getTexture();

        if (baseTexture) {
            SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
            SDL_Texture* copiedTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                                           SDL_TEXTUREACCESS_TARGET, width, height);
            if (copiedTexture) {
                SDL_SetRenderTarget(renderer, copiedTexture);
                SDL_RenderCopy(renderer, baseTexture, nullptr, nullptr);
                SDL_SetRenderTarget(renderer, oldTarget);
                textureId = resourceManager->registerTexture(copiedTexture);
                texture = copiedTexture;
            } else {
                SDL_SetRenderTarget(renderer, oldTarget);
                Log::error("[BackgroundWidget] Failed to create target texture for color background");
                return;
            }
        } else {
            Log::error("[BackgroundWidget] Failed to create base texture for color background");
            return;
        }
    } else if (useImage) {
        SDL_Texture* baseTexture = resourceManager->getTexture(imageName);
        if (!baseTexture) {
            Log::error("[BackgroundWidget] Texture not found: ", imageName);
            if (imageMode == BackgroundImageMode::SCROLL_H || imageMode == BackgroundImageMode::SCROLL_V) {
                useImage = false;
                useColor = true;
                backgroundColor = {100, 180, 220, 255};
                updateTexture();
            }
            return;
        }

        int imgW, imgH;
        SDL_QueryTexture(baseTexture, nullptr, nullptr, &imgW, &imgH);

        if (imageMode == BackgroundImageMode::SCROLL_H || imageMode == BackgroundImageMode::SCROLL_V) {
            texture = nullptr;
            textureId = "";
            createScrollChild();
        } else {
            SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
            SDL_Texture* targetTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                                           SDL_TEXTUREACCESS_TARGET, width, height);
            if (!targetTexture) {
                SDL_SetRenderTarget(renderer, oldTarget);
                Log::error("[BackgroundWidget] Failed to create target texture for image background");
                return;
            }
            SDL_SetTextureBlendMode(targetTexture, SDL_BLENDMODE_BLEND);
            SDL_SetRenderTarget(renderer, targetTexture);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);

            if (imageMode == BackgroundImageMode::STRETCH) {
                SDL_Rect dstRect = {0, 0, width, height};
                SDL_RenderCopy(renderer, baseTexture, nullptr, &dstRect);
            } else if (imageMode == BackgroundImageMode::TILE) {
                for (int y = 0; y < height; y += imgH) {
                    for (int x = 0; x < width; x += imgW) {
                        SDL_Rect dstRect = {x, y, imgW, imgH};
                        SDL_Rect srcRect = {0, 0, imgW, imgH};
                        if (x + imgW > width) {
                            srcRect.w = width - x;
                            dstRect.w = width - x;
                        }
                        if (y + imgH > height) {
                            srcRect.h = height - y;
                            dstRect.h = height - y;
                        }
                        SDL_RenderCopy(renderer, baseTexture, &srcRect, &dstRect);
                    }
                }
            } else if (imageMode == BackgroundImageMode::FIT) {
                float scaleX = static_cast<float>(width) / imgW;
                float scaleY = static_cast<float>(height) / imgH;
                float scale = std::min(scaleX, scaleY);
                int scaledW = static_cast<int>(imgW * scale);
                int scaledH = static_cast<int>(imgH * scale);
                int offsetX = (width - scaledW) / 2;
                int offsetY = (height - scaledH) / 2;
                SDL_Rect dstRect = {offsetX, offsetY, scaledW, scaledH};
                SDL_RenderCopy(renderer, baseTexture, nullptr, &dstRect);
            }

            SDL_SetRenderTarget(renderer, oldTarget);
            textureId = resourceManager->registerTexture(targetTexture);
            texture = targetTexture;
        }
    } else {
        ImageRenderer imageRenderer(renderer, width, height);
        imageRenderer.drawColor({0, 0, 0, 0});
        SDL_Texture* baseTexture = imageRenderer.getTexture();

        if (baseTexture) {
            SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
            SDL_Texture* copiedTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                                           SDL_TEXTUREACCESS_TARGET, width, height);
            if (copiedTexture) {
                SDL_SetRenderTarget(renderer, copiedTexture);
                SDL_RenderCopy(renderer, baseTexture, nullptr, nullptr);
                SDL_SetRenderTarget(renderer, oldTarget);
                textureId = resourceManager->registerTexture(copiedTexture);
                texture = copiedTexture;
            } else {
                SDL_SetRenderTarget(renderer, oldTarget);
            }
        }
    }

    auto* element = uiManager->findElementByName(uiElementId);
    if (element) {
        element->texture = texture;
        element->textureId = textureId;
        element->rect = rect;
        currentTextureId = textureId;
    }
}
