#include "ButtonWidget.h"
#include "../../ui/uiManager.h"
#include "../../resource/resourceManager.h"
#include "../../rendering/TextRenderer.h"
#include "../../rendering/ImageRenderer.h"
#include "../../animation/Animator.h"
#include "../../animation/AnimationManager.h"
#include "../../scene.h"  // MOUSE_CLICK_EVENT 접근용
#include "../../utils/logger.h"

ButtonWidget::ButtonWidget(UiManager* uiMgr, ResourceManager* resMgr,
                           SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                           const std::string& normalImg, const std::string& pressedImg,
                           const SDL_Rect& rect,
                           AnimationManager* animMgr)
    : UIWidget(uiMgr, resMgr),
      normalImageName(normalImg),
      pressedImageName(pressedImg),
      useNinePatch(false),
      useThreePatch(false),
      buttonText(""),
      fontSize(16),
      textColor({255, 255, 255, 255}),
      state(NORMAL),
      uiElementId(""),
      iconImageName(""),
      iconElementId(""),
      iconSrcW(0),
      iconSrcH(0),
      iconDisplayW(0),
      iconDisplayH(0),
      iconPadding(0),
      iconTextSpacing(0),
      currentTextureId(""),
      renderer(sdlRenderer),
      textRenderer(txtRenderer),
      animationManager(animMgr) {
    // UIElement 자동 생성 (rect 포함)
    UIElement element;
    element.name = "";  // 빈 이름으로 자동 ID 생성
    element.texture = nullptr;
    element.rect = rect;  // 생성자에서 받은 rect 사용
    element.clickable = true;
    element.visible = true;
    element.scale = 1.0f;
    element.alpha = 1.0f;
    element.rotation = 0.0f;
    uiElementId = uiManager->addUIAndGetId(element);
    // rect가 설정된 후 텍스처 업데이트 (크기가 0이면 스킵됨)
    updateTexture();
}

ButtonWidget::~ButtonWidget() {
    if (!iconElementId.empty() && uiManager) {
        uiManager->removeUI(iconElementId);
        iconElementId.clear();
    }
}

void ButtonWidget::setRect(const SDL_Rect& rect) {
    if (uiElementId.empty()) return;
    auto* element = uiManager->findElementByName(uiElementId);
    if (element) {
        element->rect = rect;
        updateIconElement();
        updateTexture();
    }
}

void ButtonWidget::setRect(int x, int y, int w, int h) {
    setRect(SDL_Rect{x, y, w, h});
}

void ButtonWidget::setOnClick(std::function<void()> callback) {
    onClickCallback = callback;
}

void ButtonWidget::setState(State newState) {
    if (state == newState) return;
    state = newState;
    // 상태 변경 시 텍스처 업데이트 없음 (애니메이션만 사용)
}

void ButtonWidget::setEnabled(bool enabled) {
    if (enabled) {
        if (state == DISABLED) {
            setState(NORMAL);
        }
    } else {
        setState(DISABLED);
    }
}

void ButtonWidget::setText(const std::string& text) {
    if (buttonText == text) return;
    buttonText = text;
    updateTexture();
}

void ButtonWidget::setFontSize(int size) {
    if (fontSize == size) return;
    fontSize = size;
    updateTexture();
}

void ButtonWidget::setTextColor(SDL_Color color) {
    if (textColor.r == color.r && textColor.g == color.g && 
        textColor.b == color.b && textColor.a == color.a) {
        return;
    }
    textColor = color;
    updateTexture();
}

void ButtonWidget::updateTexture() {
    if (uiElementId.empty()) return;
    
    // 기존 텍스처 해제
    if (!currentTextureId.empty()) {
        resourceManager->unregisterTexture(currentTextureId);
        currentTextureId = "";
    }
    
    // 항상 normalImage 사용 (상태에 관계없이 텍스처 변경 없음)
    std::string imageName = normalImageName;
    
    // UIElement에서 버튼 크기 가져오기
    auto* element = uiManager->findElementByName(uiElementId);
    if (!element) {
        // UIElement가 없으면 기본 이미지만 사용
        uiManager->changeTexture(uiElementId, imageName);
        return;
    }
    
    int buttonWidth = element->rect.w;
    int buttonHeight = element->rect.h;
    
    // 크기가 0이면 텍스처를 생성하지 않음
    if (buttonWidth <= 0 || buttonHeight <= 0) {
        return;
    }
    
    // 원본 텍스처 가져오기
    SDL_Texture* baseTexture = resourceManager->getTexture(imageName);
    if (!baseTexture) {
        Log::error("[ButtonWidget] Base texture not found: ", imageName);
        uiManager->changeTexture(uiElementId, imageName);
        return;
    }
    
    Log::info("[ButtonWidget] Image name: ", imageName, ", useNinePatch: ", useNinePatch ? "true" : "false",
              ", useThreePatch: ", useThreePatch ? "true" : "false", ", button size: ", buttonWidth, "x", buttonHeight);
    
    SDL_Texture* backgroundTexture = nullptr;
    bool usePatch = useNinePatch || useThreePatch;
    
    if (useNinePatch) {
        backgroundTexture = resourceManager->createNinePatchTexture(baseTexture, buttonWidth, buttonHeight);
        if (!backgroundTexture) {
            Log::error("[ButtonWidget] Failed to create nine-patch texture for: ", imageName);
            uiManager->changeTexture(uiElementId, imageName);
            return;
        }
        Log::info("[ButtonWidget] Nine-patch texture created successfully");
    } else if (useThreePatch) {
        backgroundTexture = resourceManager->createThreePatchTexture(baseTexture, buttonWidth, buttonHeight);
        if (!backgroundTexture) {
            Log::error("[ButtonWidget] Failed to create three-patch texture for: ", imageName);
            uiManager->changeTexture(uiElementId, imageName);
            return;
        }
        Log::info("[ButtonWidget] Three-patch texture created successfully");
    }
    
    // 텍스트가 없으면 배경만 사용
    if (buttonText.empty() || !textRenderer || !renderer) {
        if (usePatch && backgroundTexture) {
            // 9패치 텍스처 등록
            currentTextureId = resourceManager->registerTexture(backgroundTexture);
            element->texture = backgroundTexture;
            element->textureId = currentTextureId;
        } else {
            uiManager->changeTexture(uiElementId, imageName);
        }
        return;
    }
    
    // ImageRenderer를 사용해 이미지 위에 텍스트 그리기
    ImageRenderer imageRenderer(renderer, buttonWidth, buttonHeight);
    
    if (usePatch && backgroundTexture) {
        // 패치 텍스처를 ImageRenderer에 그리기 (이미 버튼 크기로 생성됨)
        // drawImage를 사용하면 needsUpdate가 자동으로 설정됨
        int texW, texH;
        SDL_QueryTexture(backgroundTexture, nullptr, nullptr, &texW, &texH);
        // 전체 텍스처를 그리기 (이미 버튼 크기로 생성되었으므로)
        imageRenderer.drawImage(backgroundTexture, 0, 0, 0, 0, texW, texH);
    } else {
        // 일반 이미지를 ImageRenderer에 그리기 (전체 텍스처를 버튼 크기로 스케일하여 채우기)
        int texW, texH;
        SDL_QueryTexture(baseTexture, nullptr, nullptr, &texW, &texH);
        imageRenderer.drawImageScaled(baseTexture, 0, 0, buttonWidth, buttonHeight, 0, 0, texW, texH);
    }
    
    // 텍스트: 아이콘 있으면 아이콘 오른쪽부터, 없으면 가운데 정렬
    int textX = 0, textY = 0;
    int textW = 0, textH = 0;
    if (textRenderer->getTextSize(buttonText, fontSize, &textW, &textH)) {
        if (!iconImageName.empty() && iconDisplayW > 0) {
            textX = iconPadding + iconDisplayW + iconTextSpacing;
            textY = (buttonHeight - textH) / 2;
        } else {
            textX = (buttonWidth - textW) / 2;
            textY = (buttonHeight - textH) / 2;
        }
    }
    if (textX < 0) textX = 0;
    if (textY < 0) textY = 0;
    
    imageRenderer.drawText(buttonText, textX, textY, fontSize, textColor, textRenderer);
    
    // 최종 텍스처 가져오기
    SDL_Texture* finalTexture = imageRenderer.getTexture();
    if (finalTexture) {
        // 텍스처를 복사해서 ResourceManager에 등록 (ImageRenderer가 소유하므로)
        // 텍스처 복사 (ImageRenderer가 소유하므로)
        int texW, texH;
        SDL_QueryTexture(finalTexture, nullptr, nullptr, &texW, &texH);
        SDL_Texture* copiedTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, texW, texH);
        if (copiedTexture) {
            // 투명도 유지를 위한 블렌딩 모드 설정
            SDL_SetTextureBlendMode(copiedTexture, SDL_BLENDMODE_BLEND);
            
            SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
            SDL_SetRenderTarget(renderer, copiedTexture);
            
            // 투명색으로 클리어 (투명도 유지)
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);
            
            SDL_RenderCopy(renderer, finalTexture, nullptr, nullptr);
            SDL_SetRenderTarget(renderer, oldTarget);
            
            // ResourceManager에 등록 (자동 ID 생성)
            currentTextureId = resourceManager->registerTexture(copiedTexture);
            
            // UIElement의 텍스처 변경
            element->texture = copiedTexture;
            element->textureId = currentTextureId;  // 텍스처 ID 설정
            
            // 패치 배경 텍스처는 ImageRenderer에 그려졌으므로 더 이상 필요 없음
            if (usePatch && backgroundTexture) {
                SDL_DestroyTexture(backgroundTexture);
                backgroundTexture = nullptr;
            }
        } else {
            if (usePatch && backgroundTexture) {
                SDL_DestroyTexture(backgroundTexture);
            }
            uiManager->changeTexture(uiElementId, imageName);
        }
    } else {
        if (usePatch && backgroundTexture) {
            SDL_DestroyTexture(backgroundTexture);
        }
        uiManager->changeTexture(uiElementId, imageName);
    }
}

void ButtonWidget::handleEvent(const SDL_Event& event) {
    if (state == DISABLED) return;  // 비활성화 상태면 무시
    
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int x = event.button.x;
        int y = event.button.y;
        
        // mouse_down 시점에 버튼 영역 안에 있었는지 확인 (애니메이션 전 크기 기준)
        wasClickableOnMouseDown = isPointInside(x, y);
        
        if (!wasClickableOnMouseDown) {
            return;  // 영역 밖이면 무시
        }
        
        isMouseDown = true;
        // 상태만 변경 (텍스처 변경 없음)
        state = PRESSED;
        
        // 스케일 애니메이션: 1.0 -> 0.7 (100ms)
        if (animationManager) {
            auto* element = uiManager->findElementByName(uiElementId);
            if (element) {
                float currentScale = element->scale;
                auto anim = std::make_shared<Animator>(uiElementId);
                anim->scale(currentScale, 0.7f, 100);  // 현재 스케일에서 0.7로
                animationManager->add(uiElementId, anim);
            }
        }
    }
    else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (isMouseDown) {
            // 상태만 복구 (클릭 처리는 mouse_click에서 처리)
            state = NORMAL;
            isMouseDown = false;
            // wasClickableOnMouseDown은 mouse_click에서 리셋 (아직 mouse_click이 올 수 있음)
            
            // 스케일 애니메이션: 0.7 -> 1.0 (100ms)
            if (animationManager) {
                auto* element = uiManager->findElementByName(uiElementId);
                if (element) {
                    float currentScale = element->scale;
                    auto anim = std::make_shared<Animator>(uiElementId);
                    anim->scale(currentScale, 1.0f, 100);  // 현재 스케일에서 1.0으로
                    animationManager->add(uiElementId, anim);
                }
            }
        }
    }
    else if (event.type == MOUSE_CLICK_EVENT && event.user.code == SDL_BUTTON_LEFT) {
        // mouse_down 시점에 클릭 가능했으면 클릭 처리 (애니메이션으로 작아져도 처리)
        if (wasClickableOnMouseDown) {
            state = NORMAL;
            wasClickableOnMouseDown = false;  // 여기서 리셋
            onClick();
        }
    }
    else if (event.type == SDL_MOUSEMOTION) {
        if (isMouseDown) {
            // 드래그 중 상태 관리 (텍스처 변경 없음)
            if (isPointInside(event.motion.x, event.motion.y)) {
                state = PRESSED;
            } else {
                state = NORMAL;
            }
        }
    }
}

void ButtonWidget::onClick() {
    // 클릭 완료 시 호출
    if (onClickCallback) {
        onClickCallback();
    }
}

void ButtonWidget::setNormalImage(const std::string& imageName, bool useNinePatchParam, bool useThreePatchParam) {
    if (normalImageName == imageName && useNinePatch == useNinePatchParam && useThreePatch == useThreePatchParam) return;
    normalImageName = imageName;
    useNinePatch = useNinePatchParam;
    useThreePatch = useThreePatchParam;
    updateTexture();
}

void ButtonWidget::setIcon(const std::string& imageName) {
    if (imageName.empty()) {
        if (!iconElementId.empty() && uiManager) {
            uiManager->removeUI(iconElementId);
            iconElementId.clear();
        }
        iconImageName.clear();
        iconSrcW = iconSrcH = 0;
        iconDisplayW = iconDisplayH = iconPadding = iconTextSpacing = 0;
        updateTexture();
        return;
    }
    iconImageName = imageName;
    SDL_Texture* iconTex = resourceManager->getTexture(imageName);
    if (!iconTex) {
        updateTexture();
        return;
    }
    int iw = 0, ih = 0;
    SDL_QueryTexture(iconTex, nullptr, nullptr, &iw, &ih);
    if (iw <= 0 || ih <= 0) {
        updateTexture();
        return;
    }
    iconSrcW = iw;
    iconSrcH = ih;

    if (iconElementId.empty()) {
        UIElement iconEl;
        iconEl.name = "";
        iconEl.texture = iconTex;
        iconEl.textureId = imageName;
        iconEl.rect = SDL_Rect{0, 0, 0, 0};
        iconEl.visible = true;
        iconEl.clickable = false;
        iconElementId = uiManager->addUIAndGetId(iconEl);
        uiManager->setParent(iconElementId, uiElementId);
    } else {
        uiManager->changeTexture(iconElementId, imageName);
    }
    updateIconElement();
    updateTexture();
}

void ButtonWidget::updateIconElement() {
    if (iconElementId.empty() || iconSrcW <= 0 || iconSrcH <= 0) return;
    auto* btnEl = uiManager->findElementByName(uiElementId);
    auto* iconEl = uiManager->findElementByName(iconElementId);
    if (!btnEl || !iconEl) return;
    int h = btnEl->rect.h;
    if (h <= 0) return;
    // 높이 기준 비율: 상하 마진 15%, 왼쪽 여백 45%, 아이콘~텍스트 10%
    int margin = static_cast<int>(h * 0.15f);
    if (margin < 1) margin = 1;
    iconPadding = static_cast<int>(h * 0.45f);
    if (iconPadding < 1) iconPadding = 1;
    iconTextSpacing = static_cast<int>(h * 0.1f);
    if (iconTextSpacing < 2) iconTextSpacing = 2;
    iconDisplayH = h - 2 * margin;
    if (iconDisplayH < 1) iconDisplayH = 1;
    iconDisplayW = static_cast<int>(static_cast<float>(iconDisplayH) * iconSrcW / iconSrcH);
    if (iconDisplayW < 1) iconDisplayW = 1;

    iconEl->rect.x = iconPadding;
    iconEl->rect.y = (h - iconDisplayH) / 2;
    if (iconEl->rect.y < 0) iconEl->rect.y = 0;
    iconEl->rect.w = iconDisplayW;
    iconEl->rect.h = iconDisplayH;
}

