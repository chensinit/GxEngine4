#include "StandardDialogWidget.h"
#include "../basic/ButtonWidget.h"
#include "../basic/TextWidget.h"
#include "../../ui/uiManager.h"
#include "../../animation/AnimationManager.h"
#include "../../animation/Animator.h"
#include "../../rendering/TextRenderer.h"
#include "../../rendering/ImageRenderer.h"
#include "../WidgetManager.h"
#include "../../utils/logger.h"
#include <memory>

StandardDialogWidget::StandardDialogWidget(UiManager* uiMgr, ResourceManager* resMgr,
                                           SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                                           AnimationManager* animMgr, WidgetManager* wmgr)
    : UIWidget(uiMgr, resMgr),
      isVisible(false),
      currentIconName(""),
      currentTitle(""),
      currentDescription(""),
      renderer(sdlRenderer),
      textRenderer(txtRenderer),
      animationManager(animMgr),
      widgetManager(wmgr) {
    
    // 배경 UIElement 생성
    createBackgroundElement();
    
    // 초기에는 숨김 상태 (자식들도 함께 숨김)
    auto* bgElement = uiManager->findElementByName(bgElementId);
    if (bgElement) {
        bgElement->visible = false;
        bgElement->scale = 1.0f;  // 초기 크기 100% (애니메이션에서 0.5로 시작)
    }
    
    // 내부 위젯들 생성 (배경이 숨김 상태이므로 자식들도 자동으로 숨김)
    createIconElement();
    createTitleWidget();
    createDescriptionWidget();
    createButtons();
}

StandardDialogWidget::~StandardDialogWidget() {
    // 버튼들은 WidgetManager가 소유하므로 여기서 해제하지 않음
    // TextWidget들은 unique_ptr이므로 자동으로 해제됨
    // UIElement는 씬 전환 시 UiManager::clear()에서 처리됨
}

void StandardDialogWidget::createBackgroundElement() {
    // 화면 중앙에 배치될 배경 UIElement 생성
    UIElement bgElement;
    bgElement.name = "";  // 자동 ID 생성
    bgElement.rect = SDL_Rect{0, 0, 400, 300};  // 기본 크기 (나중에 조정 가능)
    bgElement.visible = false;
    bgElement.clickable = false;
    bgElement.scale = 1.0f;  // 초기 크기 100% (애니메이션에서 0.5로 시작)
    
    // 흰색 배경 텍스처 생성
    int bgWidth = bgElement.rect.w;
    int bgHeight = bgElement.rect.h;
    ImageRenderer imageRenderer(renderer, bgWidth, bgHeight);
    imageRenderer.drawColor(SDL_Color{255, 255, 255, 255});  // 흰색 배경
    
    SDL_Texture* bgTexture = imageRenderer.getTexture();
    if (bgTexture) {
        // 텍스처를 복사하여 등록 (ImageRenderer가 소유하므로)
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
            bgElement.texture = copiedTexture;
            bgElement.textureId = textureId;
        } else {
            bgElement.texture = nullptr;
            Log::error("[StandardDialogWidget] Failed to copy background texture: ", SDL_GetError());
        }
    } else {
        bgElement.texture = nullptr;
        Log::error("[StandardDialogWidget] Failed to create background texture");
    }
    
    bgElementId = uiManager->addUIAndGetId(bgElement);
    uiElementId = bgElementId;  // getUIElementIdentifier()에서 사용
}

void StandardDialogWidget::createIconElement() {
    // 아이콘 UIElement 생성 (좌상단)
    UIElement iconElement;
    iconElement.name = "";  // 자동 ID 생성
    iconElement.texture = nullptr;
    iconElement.rect = SDL_Rect{10, 10, 50, 50};  // 좌상단에 배치 (로컬 좌표)
    iconElement.visible = true;  // 기본값 true (부모의 visible에 따라 자동 처리됨)
    iconElement.clickable = false;
    
    iconElementId = uiManager->addUIAndGetId(iconElement);
    uiManager->setParent(iconElementId, bgElementId);
}

void StandardDialogWidget::createTitleWidget() {
    // 타이틀 TextWidget 생성 (아이콘 오른쪽)
    titleWidget = std::make_unique<TextWidget>(
        uiManager, resourceManager, renderer, textRenderer,
        "", 24, SDL_Color{0, 0, 0, 255}, 0, -1  // 검은색, 24px
    );
    
    // 타이틀의 UIElement를 배경의 자식으로 설정 (아이콘 오른쪽)
    std::string titleElementId = titleWidget->getUIElementId();
    
    // 부모를 먼저 설정한 후 위치 설정
    uiManager->setParent(titleElementId, bgElementId);
    uiManager->setRect(titleElementId, 70, 20, 0, 0);  // 아이콘 오른쪽 (10 + 50 + 10), 아이콘과 같은 높이
    
    // visible은 기본값 true (부모의 visible에 따라 자동 처리됨)
}

void StandardDialogWidget::createDescriptionWidget() {
    // 설명 TextWidget 생성 (아이콘 아래)
    descriptionWidget = std::make_unique<TextWidget>(
        uiManager, resourceManager, renderer, textRenderer,
        "", 16, SDL_Color{64, 64, 64, 255}, 350, -1  // 회색, 16px, wrap 350px
    );
    
    // 설명의 UIElement를 배경의 자식으로 설정 (아이콘 아래)
    std::string descElementId = descriptionWidget->getUIElementId();
    
    // 부모를 먼저 설정한 후 위치 설정
    uiManager->setParent(descElementId, bgElementId);
    uiManager->setRect(descElementId, 10, 70, 0, 0);  // 아이콘 아래 (10 + 50 + 10), 좌측 정렬
    
    // visible은 기본값 true (부모의 visible에 따라 자동 처리됨)
}

void StandardDialogWidget::createButtons() {
    // 버튼 크기 설정 (100x40)
    int buttonWidth = 100;
    int buttonHeight = 40;
    int buttonSpacing = 10;  // 버튼 간 간격
    int buttonY = 250;  // 설명 아래 (대략 70 + 설명 높이 + 여백)
    
    // OK 버튼 생성 (기존 이미지 사용: like, robot, rect 포함)
    auto okButtonPtr = std::make_unique<ButtonWidget>(
        uiManager, resourceManager, renderer, textRenderer,
        "like", "robot",  // normalImage, pressedImage
        SDL_Rect{10 + buttonWidth + buttonSpacing, buttonY, buttonWidth, buttonHeight},  // rect
        animationManager  // AnimationManager 전달
    );
    okButtonPtr->setText("OK");
    okButtonPtr->setOnClick([this]() {
        if (onOkCallback) {
            onOkCallback();
        }
        hide();
    });
    
    // Cancel 버튼 생성 (기존 이미지 사용: like, robot, rect 포함)
    auto cancelButtonPtr = std::make_unique<ButtonWidget>(
        uiManager, resourceManager, renderer, textRenderer,
        "like", "robot",  // normalImage, pressedImage
        SDL_Rect{10, buttonY, buttonWidth, buttonHeight},  // rect
        animationManager  // AnimationManager 전달
    );
    cancelButtonPtr->setText("Cancel");
    cancelButtonPtr->setOnClick([this]() {
        if (onCancelCallback) {
            onCancelCallback();
        }
        hide();
    });
    
    // 버튼들의 UIElement를 배경의 자식으로 설정 (위젯 ID 사용)
    std::string okElementId = okButtonPtr->getUIElementIdentifier();
    std::string cancelElementId = cancelButtonPtr->getUIElementIdentifier();
    
    // 부모 설정
    uiManager->setParent(cancelElementId, bgElementId);
    uiManager->setParent(okElementId, bgElementId);
    
    // visible은 기본값 true (부모의 visible에 따라 자동 처리됨)
    
    // 버튼들을 WidgetManager에 등록 (클릭 처리를 위해)
    // 소유권은 WidgetManager로 이동하지만, 포인터는 유지하여 나중에 접근 가능
    if (widgetManager) {
        okButton = okButtonPtr.get();
        cancelButton = cancelButtonPtr.get();
        
        // 버튼 이름 생성 (bgElementId 기반)
        std::string okButtonName = bgElementId + "_ok";
        std::string cancelButtonName = bgElementId + "_cancel";
        
        widgetManager->addWidget(okButtonName, std::move(okButtonPtr));
        widgetManager->addWidget(cancelButtonName, std::move(cancelButtonPtr));
    } else {
        // WidgetManager가 없으면 포인터만 저장 (소유권은 여기서 유지)
        okButton = okButtonPtr.release();
        cancelButton = cancelButtonPtr.release();
    }
}

void StandardDialogWidget::setContent(const std::string& iconName, const std::string& title, const std::string& description) {
    currentIconName = iconName;
    currentTitle = title;
    currentDescription = description;
    
    // 아이콘 설정
    if (!iconName.empty()) {
        uiManager->changeTexture(iconElementId, iconName);
        auto* iconElement = uiManager->findElementByName(iconElementId);
        if (iconElement) {
            iconElement->visible = true;
        }
    } else {
        auto* iconElement = uiManager->findElementByName(iconElementId);
        if (iconElement) {
            iconElement->visible = false;
        }
    }
    
    // 타이틀 설정
    if (titleWidget) {
        titleWidget->setText(title);
        // 타이틀 위치는 createTitleWidget()에서 이미 설정됨
        // 중앙 정렬은 텍스트 크기를 알 수 없으므로 일단 기본 위치 유지
    }
    
    // 설명 설정
    if (descriptionWidget) {
        descriptionWidget->setText(description);
    }
}

void StandardDialogWidget::show() {
    if (isVisible) return;
    
    isVisible = true;
    auto* bgElement = uiManager->findElementByName(bgElementId);
    if (!bgElement) return;
    
    // 화면 중앙에 배치
    int screenWidth, screenHeight;
    SDL_GetRendererOutputSize(renderer, &screenWidth, &screenHeight);
    bgElement->rect.x = (screenWidth - bgElement->rect.w) / 2;
    bgElement->rect.y = (screenHeight - bgElement->rect.h) / 2;
    
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

void StandardDialogWidget::hide() {
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

