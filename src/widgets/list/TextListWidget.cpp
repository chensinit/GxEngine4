#include "TextListWidget.h"
#include "../../ui/uiManager.h"
#include "../../scene.h"  // MOUSE_CLICK_EVENT 접근용
#include "../../rendering/TextRenderer.h"
#include "../../resource/resourceManager.h"
#include <sstream>

TextListWidget::TextListWidget(UiManager* uiMgr, ResourceManager* resMgr,
                               SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                               int itemH, int size, SDL_Color color,
                               const SDL_Rect& rect, float scale, float rotation,
                               float alpha, bool visible, bool clickable)
    : UIWidget(uiMgr, resMgr),
      itemHeight(itemH),
      fontSize(size),
      textColor(color),
      scrollOffset(0),
      maxVisibleItems(0),
      renderer(sdlRenderer),
      textRenderer(txtRenderer),
      isDragging(false),
      lastMouseY(0),
      dragStartY(0),
      uiElementId("") {
    // UIElement 생성 (렌더링 영역 정의용)
    UIElement element;
    element.name = "";  // 빈 이름으로 자동 ID 생성
    element.texture = nullptr;  // TextListWidget은 직접 렌더링
    element.rect = rect;
    element.visible = visible;
    element.clickable = clickable;
    element.scale = scale;
    element.rotation = rotation;
    element.alpha = alpha;
    uiElementId = uiManager->addUIAndGetId(element);
    
    // maxVisibleItems 계산
    if (rect.h > 0) {
        maxVisibleItems = rect.h / itemHeight + 1;
    }
}

TextListWidget::~TextListWidget() {
    clearItemTextures();
}

void TextListWidget::setItems(const std::vector<std::string>& newItems) {
    items = newItems;
    updateItemTextures();
    // 스크롤 오프셋이 범위를 벗어나지 않도록 조정
    auto* element = uiManager->findElementByName(uiElementId);
    if (element) {
        int widgetH = element->rect.h;
        int maxScroll = static_cast<int>(items.size()) * itemHeight - widgetH;
        if (maxScroll < 0) maxScroll = 0;
        if (scrollOffset > maxScroll) scrollOffset = maxScroll;
        if (scrollOffset < 0) scrollOffset = 0;
    } else {
        scrollOffset = 0;
    }
}

void TextListWidget::addItem(const std::string& item) {
    items.push_back(item);
    // 새 항목의 텍스처 생성
    SDL_Texture* texture = textRenderer->renderText(renderer, item, fontSize, textColor);
    if (texture) {
        // ResourceManager에 등록
        std::string textureId = resourceManager->registerTexture(texture);
        
        int w, h;
        SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
        ItemTexture itemTex;
        itemTex.text = item;
        itemTex.texture = texture;
        itemTex.textureId = textureId;
        itemTex.width = w;
        itemTex.height = h;
        itemTextures.push_back(itemTex);
    }
}

void TextListWidget::clearItems() {
    items.clear();
    clearItemTextures();
    scrollOffset = 0;
}

void TextListWidget::setItemHeight(int height) {
    itemHeight = height;
}

void TextListWidget::setFontSize(int size) {
    if (fontSize == size) return;
    fontSize = size;
    updateItemTextures();
}

void TextListWidget::setTextColor(SDL_Color color) {
    if (textColor.r == color.r && textColor.g == color.g && 
        textColor.b == color.b && textColor.a == color.a) {
        return;
    }
    textColor = color;
    updateItemTextures();
}

void TextListWidget::setScrollOffset(int offset) {
    scrollOffset = offset;
    
    // 위젯 크기 가져오기
    auto* element = uiManager->findElementByName(uiElementId);
    if (!element) {
        scrollOffset = 0;
        return;
    }
    
    int widgetH = element->rect.h;
    
    // 범위 제한
    int maxScroll = static_cast<int>(items.size()) * itemHeight - widgetH;
    if (maxScroll < 0) maxScroll = 0;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;
    if (scrollOffset < 0) scrollOffset = 0;
}

void TextListWidget::scrollToTop() {
    scrollOffset = 0;
}

void TextListWidget::scrollToBottom() {
    auto* element = uiManager->findElementByName(uiElementId);
    if (!element) {
        scrollOffset = 0;
        return;
    }
    int widgetH = element->rect.h;
    int maxScroll = static_cast<int>(items.size()) * itemHeight - widgetH;
    if (maxScroll < 0) maxScroll = 0;
    scrollOffset = maxScroll;
}

void TextListWidget::handleEvent(const SDL_Event& event) {
    // 위젯의 위치와 크기 가져오기
    auto* element = uiManager->findElementByName(uiElementId);
    if (!element) return;
    
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int x = event.button.x;
        int y = event.button.y;
        
        // mouse_down 시점에 버튼 영역 안에 있었는지 확인
        wasDraggableOnMouseDown = isPointInside(x, y);
        
        if (!wasDraggableOnMouseDown) {
            return;  // 영역 밖이면 무시
        }
        
        isDragging = true;
        lastMouseY = event.button.y;
        dragStartY = event.button.y;
    }
    else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        // mouse_down 시점에 드래그 가능했으면 mouse_up 처리 (범위를 벗어나도 처리)
        if (wasDraggableOnMouseDown) {
            isDragging = false;
            wasDraggableOnMouseDown = false;  // 리셋
        }
    }
    else if (event.type == SDL_MOUSEMOTION) {
        if (isDragging) {
            int mouseY = event.motion.y;
            int deltaY = mouseY - lastMouseY;
            setScrollOffset(scrollOffset - deltaY);  // 마우스가 아래로 가면 스크롤도 아래로
            lastMouseY = mouseY;
        }
    }
    else if (event.type == MOUSE_CLICK_EVENT) {
        // 클릭 이벤트는 무시 (리스트는 드래그만 처리)
        return;
    }
}

void TextListWidget::render(SDL_Renderer* sdlRenderer) {
    if (items.empty()) return;
    
    // 위젯의 위치와 크기 가져오기 (UIElement에서)
    auto* element = uiManager->findElementByName(uiElementId);
    if (!element || !element->visible) return;
    
    // 월드 위치 계산 (부모 위치 포함)
    int widgetX, widgetY;
    uiManager->getWorldPosition(uiElementId, widgetX, widgetY);
    int widgetW = element->rect.w;
    int widgetH = element->rect.h;
    
    // 화면에 보이는 최대 항목 수 계산
    maxVisibleItems = widgetH / itemHeight + 1;
    
    // 스크롤 범위 제한
    int maxScroll = static_cast<int>(items.size()) * itemHeight - widgetH;
    if (maxScroll < 0) maxScroll = 0;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;
    if (scrollOffset < 0) scrollOffset = 0;
    
    // 보이는 항목 범위 계산
    int startIndex = scrollOffset / itemHeight;
    int endIndex = startIndex + maxVisibleItems + 1;
    if (endIndex > static_cast<int>(items.size())) {
        endIndex = static_cast<int>(items.size());
    }
    
    // 각 항목 렌더링
    for (int i = startIndex; i < endIndex; i++) {
        if (i >= static_cast<int>(itemTextures.size())) continue;
        
        const ItemTexture& itemTex = itemTextures[i];
        if (!itemTex.texture) continue;
        
        // 항목의 Y 위치 계산
        int itemY = widgetY + (i * itemHeight) - scrollOffset;
        
        // 렌더링 위치
        SDL_Rect dstRect = {
            widgetX,
            itemY,
            itemTex.width < widgetW ? itemTex.width : widgetW,
            itemTex.height < itemHeight ? itemTex.height : itemHeight
        };
        
        // 텍스처 소스 rect (전체 텍스처 사용)
        SDL_Rect srcRect = {
            0,
            0,
            itemTex.width,
            itemTex.height
        };
        
        SDL_RenderCopy(sdlRenderer, itemTex.texture, &srcRect, &dstRect);
    }
}

void TextListWidget::updateItemTextures() {
    clearItemTextures();
    
    for (const auto& item : items) {
        SDL_Texture* texture = textRenderer->renderText(renderer, item, fontSize, textColor);
        if (texture) {
            // ResourceManager에 등록
            std::string textureId = resourceManager->registerTexture(texture);
            
            int w, h;
            SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
            ItemTexture itemTex;
            itemTex.text = item;
            itemTex.texture = texture;
            itemTex.textureId = textureId;
            itemTex.width = w;
            itemTex.height = h;
            itemTextures.push_back(itemTex);
        }
    }
}

void TextListWidget::clearItemTextures() {
    for (auto& itemTex : itemTextures) {
        if (!itemTex.textureId.empty()) {
            // ResourceManager를 통해 텍스처 해제 (참조 카운팅)
            resourceManager->unregisterTexture(itemTex.textureId);
        }
    }
    itemTextures.clear();
}

