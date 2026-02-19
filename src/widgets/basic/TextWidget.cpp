#include "TextWidget.h"
#include "../../ui/uiManager.h"
#include "../../rendering/TextRenderer.h"

TextWidget::TextWidget(UiManager* uiMgr, ResourceManager* resMgr,
                       SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                       const std::string& initialText, int size, SDL_Color color,
                       int wrapW, int maxH,
                       const std::string& align, int contX, int contY, int contW, int contH)
    : UIWidget(uiMgr, resMgr),
      text(initialText),
      fontSize(size),
      textColor(color),
      currentTexture(nullptr),
      currentTextureName(""),
      uiElementId(""),
      wrapWidth(wrapW),
      maxHeight(maxH),
      textAlign(align),
      containerRect{contX, contY, contW, contH},
      renderer(sdlRenderer),
      textRenderer(txtRenderer) {
    
    // 초기 텍스처 생성
    updateTexture();
}

TextWidget::~TextWidget() {
    // ResourceManager에 텍스처 해제 요청 (참조 카운팅으로 관리)
    if (!currentTextureName.empty() && resourceManager) {
        resourceManager->unregisterTexture(currentTextureName);
        currentTextureName = "";
        currentTexture = nullptr;
        
        // UIElement에서 참조 제거 (안전하게)
        if (uiManager && !uiElementId.empty()) {
            auto* element = uiManager->findElementByName(uiElementId);
            if (element && element->texture == currentTexture) {
                element->texture = nullptr;
            }
        }
    }
}

void TextWidget::updateTexture() {
    // 기존 텍스처 해제 요청
    if (!currentTextureName.empty()) {
        resourceManager->unregisterTexture(currentTextureName);
        currentTextureName = "";
        currentTexture = nullptr;
    }
    
    // 텍스트는 항상 개별 텍스처로 생성 (캐싱 없음)
    currentTexture = textRenderer->renderText(renderer, text, fontSize, textColor, wrapWidth, maxHeight);
    
    // ResourceManager에 등록 (자동 ID 생성, 이름 지정 없음)
    // currentTexture가 nullptr이면 빈 문자열 반환되고, UIElement의 texture도 nullptr이 됨 (렌더링만 스킵)
    currentTextureName = resourceManager->registerTexture(currentTexture);
    
    // UIElement 찾기 또는 생성
    if (uiElementId.empty()) {
        // 처음 생성: UiManager가 ID 생성
        UIElement newElement;
        newElement.name = "";  // 빈 문자열로 전달하면 자동 ID 생성
        newElement.texture = currentTexture;
        newElement.textureId = currentTextureName;  // 텍스처 ID 설정
        newElement.rect = SDL_Rect{0, 0, 0, 0};  // 나중에 WidgetManager에서 설정됨
        newElement.visible = true;
        uiElementId = uiManager->addUIAndGetId(newElement);  // ID 저장
    }
    
    auto* element = uiManager->findElementByName(uiElementId);
    if (element) {
        element->texture = currentTexture;
        element->textureId = currentTextureName;  // 텍스처 ID 업데이트
        
        int w, h;
        SDL_QueryTexture(currentTexture, nullptr, nullptr, &w, &h);
        element->rect.w = w;
        element->rect.h = h;
        
        // 컨테이너가 지정된 경우 정렬에 따라 위치 설정
        if (containerRect.w >= 0) {
            if (textAlign == "center") {
                element->rect.x = containerRect.x + (containerRect.w - w) / 2;
                element->rect.y = containerRect.y;
            } else {
                element->rect.x = containerRect.x;
                element->rect.y = containerRect.y;
            }
        }
    }
}

void TextWidget::setText(const std::string& newText) {
    if (text == newText) return;  // 변경 없으면 스킵
    
    text = newText;
    updateTexture();
}

void TextWidget::setFontSize(int size) {
    if (fontSize == size) return;
    
    fontSize = size;
    updateTexture();
}

void TextWidget::setTextColor(SDL_Color color) {
    if (textColor.r == color.r && textColor.g == color.g && 
        textColor.b == color.b && textColor.a == color.a) {
        return;
    }
    
    textColor = color;
    updateTexture();
}

void TextWidget::setWrapWidth(int width) {
    if (wrapWidth == width) return;
    
    wrapWidth = width;
    updateTexture();
}

void TextWidget::setMaxHeight(int height) {
    if (maxHeight == height) return;
    
    maxHeight = height;
    updateTexture();
}

