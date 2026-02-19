#include "BackgroundTextWidget.h"
#include "../../ui/uiManager.h"
#include "../../rendering/TextRenderer.h"
#include "../../rendering/ImageRenderer.h"
#include "../../resource/resourceManager.h"
#include "../../utils/logger.h"

static const int BACKGROUND_PADDING = 8;

// --- 헬퍼: 소스 텍스처를 영구 텍스처로 복사 (ResourceManager에 등록)
static SDL_Texture* copyToPersistentTexture(SDL_Renderer* renderer, SDL_Texture* source,
                                            int w, int h, ResourceManager* resMgr,
                                            std::string& outTextureId) {
    if (!source || w <= 0 || h <= 0) return nullptr;
    SDL_Texture* dest = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                          SDL_TEXTUREACCESS_TARGET, w, h);
    if (!dest) return nullptr;
    SDL_SetTextureBlendMode(dest, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, dest);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, source, nullptr, nullptr);
    SDL_SetRenderTarget(renderer, oldTarget);
    outTextureId = resMgr->registerTexture(dest);
    return dest;
}

BackgroundTextWidget::BackgroundTextWidget(UiManager* uiMgr, ResourceManager* resMgr,
                                           SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                                           const std::string& initialText, int size,
                                           SDL_Color txtColor, SDL_Color bgColor,
                                           int wrapW, int maxH)
    : UIWidget(uiMgr, resMgr),
      text(initialText),
      fontSize(size),
      textColor(txtColor),
      backgroundColor(bgColor),
      backgroundImageName(""),
      useNinePatch(false),
      useThreePatch(false),
      textElementId(""),
      backgroundElementId(""),
      textTexture(nullptr),
      backgroundTexture(nullptr),
      textTextureId(""),
      backgroundTextureId(""),
      wrapWidth(wrapW),
      maxHeight(maxH),
      fixedBackgroundWidth(0),
      fixedBackgroundHeight(0),
      renderer(sdlRenderer),
      textRenderer(txtRenderer) {
    updateTextures();
}

BackgroundTextWidget::~BackgroundTextWidget() {
    if (!textElementId.empty()) uiManager->removeUI(textElementId);
    if (!backgroundElementId.empty()) uiManager->removeUI(backgroundElementId);
    if (!textTextureId.empty() && resourceManager) resourceManager->unregisterTexture(textTextureId);
    if (!backgroundTextureId.empty() && resourceManager) resourceManager->unregisterTexture(backgroundTextureId);
}

void BackgroundTextWidget::updateTextures() {
    // 1. 기존 요소/텍스처 제거 및 위치 저장
    int savedX = 0, savedY = 0, oldBgW = 0;
    if (uiManager && !backgroundElementId.empty()) {
        if (UIElement* old = uiManager->findElementByName(backgroundElementId)) {
            savedX = old->rect.x;
            savedY = old->rect.y;
            oldBgW = old->rect.w;
        }
    }
    if (uiManager) {
        if (!textElementId.empty()) { uiManager->removeUI(textElementId); textElementId = ""; }
        if (!backgroundElementId.empty()) { uiManager->removeUI(backgroundElementId); backgroundElementId = ""; }
    }
    if (!textTextureId.empty()) {
        resourceManager->unregisterTexture(textTextureId);
        textTextureId = "";
        textTexture = nullptr;
    }
    if (!backgroundTextureId.empty()) {
        resourceManager->unregisterTexture(backgroundTextureId);
        backgroundTextureId = "";
        backgroundTexture = nullptr;
    }

    // 2. 텍스트 크기 계산
    textTexture = textRenderer->renderText(renderer, text, fontSize, textColor, wrapWidth, maxHeight);
    if (!textTexture) return;
    int textW, textH;
    SDL_QueryTexture(textTexture, nullptr, nullptr, &textW, &textH);
    textTextureId = resourceManager->registerTexture(textTexture);

    // 3. 배경 크기 및 텍스처 생성
    int bgW = (fixedBackgroundWidth > 0 && fixedBackgroundHeight > 0)
        ? fixedBackgroundWidth : textW + BACKGROUND_PADDING * 2;
    int bgH = (fixedBackgroundWidth > 0 && fixedBackgroundHeight > 0)
        ? fixedBackgroundHeight : textH + BACKGROUND_PADDING * 2;

    SDL_Texture* bgSrc = nullptr;
    if (!backgroundImageName.empty()) {
        SDL_Texture* base = resourceManager->getTexture(backgroundImageName);
        if (base) {
            if (useNinePatch) {
                bgSrc = resourceManager->createNinePatchTexture(base, bgW, bgH);
            } else if (useThreePatch) {
                bgSrc = resourceManager->createThreePatchTexture(base, bgW, bgH);
            } else {
                ImageRenderer ir(renderer, bgW, bgH);
                ir.drawImage(base, 0, 0, 0, 0, bgW, bgH);
                bgSrc = ir.getTexture();
            }
        }
    }
    if (!bgSrc) {
        ImageRenderer ir(renderer, bgW, bgH);
        ir.drawColor(backgroundColor);
        bgSrc = ir.getTexture();
    }
    backgroundTexture = copyToPersistentTexture(renderer, bgSrc, bgW, bgH, resourceManager, backgroundTextureId);
    if ((useNinePatch || useThreePatch) && bgSrc) SDL_DestroyTexture(bgSrc);

    // 4. 위젯 위치 계산 (textAlign 기준)
    int posX = savedX, posY = savedY;
    if (oldBgW > 0) {
        if (textAlign == "right") posX = (savedX + oldBgW) - bgW;
        else if (textAlign == "center") posX = (savedX + oldBgW / 2) - bgW / 2;
    }

    // 5. 배경 + 텍스트 UIElement 생성 및 등록
    UIElement bgEl;
    bgEl.name = "";
    bgEl.texture = backgroundTexture;
    bgEl.textureId = backgroundTextureId;
    bgEl.rect = {posX, posY, bgW, bgH};
    bgEl.visible = true;
    bgEl.clickable = false;
    backgroundElementId = uiManager->addUIAndGetId(bgEl);

    UIElement txtEl;
    txtEl.name = "";
    txtEl.texture = textTexture;
    txtEl.textureId = textTextureId;
    txtEl.rect = {BACKGROUND_PADDING, BACKGROUND_PADDING, textW, textH};
    txtEl.visible = true;
    txtEl.clickable = false;
    textElementId = uiManager->addUIAndGetId(txtEl);
    uiManager->setParent(textElementId, backgroundElementId);
}

void BackgroundTextWidget::setText(const std::string& newText) {
    if (text == newText) return;
    text = newText;
    updateTextures();
}

void BackgroundTextWidget::setFontSize(int size) {
    if (fontSize == size) return;
    fontSize = size;
    updateTextures();
}

void BackgroundTextWidget::setTextColor(SDL_Color color) {
    if (textColor.r == color.r && textColor.g == color.g && textColor.b == color.b && textColor.a == color.a) return;
    textColor = color;
    updateTextures();
}

void BackgroundTextWidget::setBackgroundColor(SDL_Color color) {
    if (backgroundColor.r == color.r && backgroundColor.g == color.g &&
        backgroundColor.b == color.b && backgroundColor.a == color.a) return;
    backgroundColor = color;
    updateTextures();
}

void BackgroundTextWidget::setWrapWidth(int width) {
    if (wrapWidth == width) return;
    wrapWidth = width;
    updateTextures();
}

void BackgroundTextWidget::setMaxHeight(int height) {
    if (maxHeight == height) return;
    maxHeight = height;
    updateTextures();
}

void BackgroundTextWidget::setFixedBackgroundSize(int w, int h) {
    if (fixedBackgroundWidth == w && fixedBackgroundHeight == h) return;
    fixedBackgroundWidth = w;
    fixedBackgroundHeight = h;
    updateTextures();
}

void BackgroundTextWidget::setTextAlign(const std::string& align) {
    if (textAlign == align) return;
    textAlign = align;
    updateTextures();
}

void BackgroundTextWidget::setBackgroundImage(const std::string& imageName, bool useNinePatchParam, bool useThreePatchParam) {
    if (backgroundImageName == imageName && useNinePatch == useNinePatchParam && useThreePatch == useThreePatchParam) return;
    backgroundImageName = imageName;
    useNinePatch = useNinePatchParam;
    useThreePatch = useThreePatchParam;
    updateTextures();
}
