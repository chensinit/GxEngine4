#include "MultiTypeListWidget.h"
#include "../basic/BackgroundTextWidget.h"
#include "../../ui/uiManager.h"
#include "../../scene.h"  // MOUSE_CLICK_EVENT 접근용
#include "../../rendering/TextRenderer.h"
#include "../../resource/resourceManager.h"
#include "../../utils/logger.h"
#include <algorithm>
#include <set>

MultiTypeListWidget::MultiTypeListWidget(UiManager* uiMgr, ResourceManager* resMgr,
                                         SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                                         int itemH, int size, SDL_Color txtColor, SDL_Color bgColor,
                                         int iconSz, int spacing,
                                         const SDL_Rect& rect, float scale, float rotation,
                                         float alpha, bool visible, bool clickable)
    : UIWidget(uiMgr, resMgr),
      itemHeight(itemH),
      fontSize(size),
      textColor(txtColor),
      backgroundColor(bgColor),
      backgroundImageName(""),
      useNinePatch(false),
      useThreePatch(false),
      iconSize(iconSz),
      iconTextSpacing(spacing),
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
    element.name = "";
    element.texture = nullptr;
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

MultiTypeListWidget::~MultiTypeListWidget() {
    clearVisibleItems();
}

void MultiTypeListWidget::setItems(const std::vector<ListItem>& newItems) {
    clearVisibleItems();
    items = newItems;
    
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
    
    updateVisibleItems();
}

void MultiTypeListWidget::addItem(const ListItem& item) {
    items.push_back(item);
    updateVisibleItems();
}

void MultiTypeListWidget::addItem(ListItemType type, const std::string& text,
                                  const std::string& iconName, const std::string& imageName) {
    addItem(ListItem(type, text, iconName, imageName));
}

void MultiTypeListWidget::clearItems() {
    clearVisibleItems();
    items.clear();
    scrollOffset = 0;
}

void MultiTypeListWidget::setItemHeight(int height) {
    if (itemHeight == height) return;
    itemHeight = height;
    updateVisibleItems();
}

void MultiTypeListWidget::setFontSize(int size) {
    if (fontSize == size) return;
    fontSize = size;
    updateVisibleItems();
}

void MultiTypeListWidget::setTextColor(SDL_Color color) {
    if (textColor.r == color.r && textColor.g == color.g && 
        textColor.b == color.b && textColor.a == color.a) {
        return;
    }
    textColor = color;
    updateVisibleItems();
}

void MultiTypeListWidget::setBackgroundColor(SDL_Color color) {
    if (backgroundColor.r == color.r && backgroundColor.g == color.g && 
        backgroundColor.b == color.b && backgroundColor.a == color.a) {
        return;
    }
    backgroundColor = color;
    updateVisibleItems();
}

void MultiTypeListWidget::setBackgroundImage(const std::string& imageName, bool useNinePatchParam, bool useThreePatchParam) {
    if (backgroundImageName == imageName && useNinePatch == useNinePatchParam && useThreePatch == useThreePatchParam) return;
    Log::info("[MultiTypeListWidget] setBackgroundImage: ", imageName, ", useNinePatch: ", useNinePatchParam, ", useThreePatch: ", useThreePatchParam);
    backgroundImageName = imageName;
    useNinePatch = useNinePatchParam;
    useThreePatch = useThreePatchParam;
    updateVisibleItems();
}

void MultiTypeListWidget::setIconSize(int size) {
    if (iconSize == size) return;
    iconSize = size;
    updateVisibleItems();
}

void MultiTypeListWidget::setIconTextSpacing(int spacing) {
    if (iconTextSpacing == spacing) return;
    iconTextSpacing = spacing;
    updateVisibleItems();
}

void MultiTypeListWidget::setScrollOffset(int offset) {
    scrollOffset = offset;
    
    // 범위 제한
    auto* element = uiManager->findElementByName(uiElementId);
    if (!element) return;
    
    int widgetH = element->rect.h;
    int maxScroll = static_cast<int>(items.size()) * itemHeight - widgetH;
    if (maxScroll < 0) maxScroll = 0;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;
    if (scrollOffset < 0) scrollOffset = 0;
    
    updateItemPositions();
    updateVisibleItems();  // 스크롤 시 보이는 항목 변경될 수 있음
}

void MultiTypeListWidget::scrollToTop() {
    scrollOffset = 0;
    updateItemPositions();
    updateVisibleItems();
}

void MultiTypeListWidget::scrollToBottom() {
    auto* element = uiManager->findElementByName(uiElementId);
    if (!element) return;
    
    int widgetH = element->rect.h;
    int maxScroll = static_cast<int>(items.size()) * itemHeight - widgetH;
    if (maxScroll < 0) maxScroll = 0;
    scrollOffset = maxScroll;
    updateItemPositions();
    updateVisibleItems();
}

void MultiTypeListWidget::handleEvent(const SDL_Event& event) {
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

void MultiTypeListWidget::update(float deltaTime) {
    // 스크롤 위치 업데이트는 handleEvent에서 처리
}

void MultiTypeListWidget::updateVisibleItems() {
    if (items.empty()) {
        clearVisibleItems();
        return;
    }
    
    auto* element = uiManager->findElementByName(uiElementId);
    if (!element) return;
    
    int viewportHeight = element->rect.h;
    if (viewportHeight <= 0) return;
    
    // 보이는 범위 계산 (단순 선형 탐색)
    int startIndex = scrollOffset / itemHeight;
    if (startIndex < 0) startIndex = 0;
    if (startIndex >= static_cast<int>(items.size())) {
        clearVisibleItems();
        return;
    }
    
    int endIndex = startIndex + (viewportHeight / itemHeight) + 2;  // 버퍼 2개
    endIndex = std::min(endIndex, static_cast<int>(items.size()) - 1);
    if (endIndex < startIndex) {
        clearVisibleItems();
        return;
    }
    
    // 현재 보이는 인덱스 집합
    std::set<int> currentVisible;
    for (int i = startIndex; i <= endIndex; i++) {
        currentVisible.insert(i);
    }
    
    // 제거: 보이지 않는 항목 제거
    for (auto it = visibleItems.begin(); it != visibleItems.end();) {
        if (currentVisible.find(it->itemIndex) == currentVisible.end()) {
            Log::info("[removeItemElement] Removing child for item index: ", it->itemIndex);
            if (it->textWidget) {
                Log::info("[MultiTypeListWidget] Removing text widget for item index: ", it->itemIndex);
                it->textWidget.reset();  // 소멸자에서 UIElement 제거됨
            }
            
            // 아이콘 UIElement 제거
            if (!it->iconElementId.empty()) {
                uiManager->removeUI(it->iconElementId);
            }
            
            // 이미지 UIElement 제거
            if (!it->imageElementId.empty()) {
                uiManager->removeUI(it->imageElementId);
            }
            
            it = visibleItems.erase(it);
        } else {
            ++it;
        }
    }
    
    // 생성: 새로 보이는 항목 생성
    for (int i = startIndex; i <= endIndex; i++) {
        bool exists = false;
        for (const auto& v : visibleItems) {
            if (v.itemIndex == i) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            createItemElement(i);
        }
    }
    
    // 위치 업데이트
    updateItemPositions();
}

void MultiTypeListWidget::createItemElement(int index) {
    if (index < 0 || index >= static_cast<int>(items.size())) return;
    
    const ListItem& item = items[index];
    VisibleItem visible;
    visible.itemIndex = index;
    
    auto* parentElement = uiManager->findElementByName(uiElementId);
    if (!parentElement) return;
    
    int maxWidth = parentElement->rect.w;
    if (maxWidth <= 0) return;
    
    int baseY = index * itemHeight;  // 스크롤 오프셋은 updateItemPositions에서 적용
    
    switch (item.type) {
        case ListItemType::CENTER_TEXT: {
            // 가운데 정렬된 텍스트
            int textMaxWidth = maxWidth - 20;  // 좌우 여백 10px씩
            auto textWidget = std::make_unique<BackgroundTextWidget>(
                uiManager, resourceManager, renderer, textRenderer,
                item.text, fontSize, textColor, backgroundColor, textMaxWidth, -1
            );
            
            // 배경 이미지 설정
            if (!backgroundImageName.empty()) {
                textWidget->setBackgroundImage(backgroundImageName, useNinePatch, useThreePatch);
            }
            
            std::string bgElementId = textWidget->getBackgroundElementId();
            auto* bgElement = uiManager->findElementByName(bgElementId);
            if (bgElement) {
                // 가운데 정렬
                int textW = bgElement->rect.w;
                bgElement->rect.x = (maxWidth - textW) / 2;
                uiManager->setParent(bgElementId, uiElementId);
            }
            
            visible.textWidget = std::move(textWidget);
            break;
        }
        
        case ListItemType::LEFT_ICON_TEXT: {
            // 왼쪽 아이콘 + 텍스트
            int textMaxWidth = maxWidth - iconSize - iconTextSpacing - 20;
            if (textMaxWidth <= 0) {
                textMaxWidth = maxWidth - 20;
            }
            
            auto textWidget = std::make_unique<BackgroundTextWidget>(
                uiManager, resourceManager, renderer, textRenderer,
                item.text, fontSize, textColor, backgroundColor, textMaxWidth, -1
            );
            
            // 배경 이미지 설정
            if (!backgroundImageName.empty()) {
                textWidget->setBackgroundImage(backgroundImageName, useNinePatch, useThreePatch);
            }
            
            std::string bgElementId = textWidget->getBackgroundElementId();
            auto* bgElement = uiManager->findElementByName(bgElementId);
            if (!bgElement) break;
            
            int textW = bgElement->rect.w;
            int textH = bgElement->rect.h;
            
            // 아이콘 생성
            if (!item.iconName.empty()) {
                SDL_Texture* iconTexture = resourceManager->getTexture(item.iconName);
                if (iconTexture) {
                    int origW, origH;
                    SDL_QueryTexture(iconTexture, nullptr, nullptr, &origW, &origH);
                    float scale = std::min(static_cast<float>(iconSize) / origW, 
                                          static_cast<float>(iconSize) / origH);
                    int iconW = static_cast<int>(origW * scale);
                    int iconH = static_cast<int>(origH * scale);
                    
                    UIElement iconElement;
                    iconElement.name = "";
                    iconElement.texture = iconTexture;
                    iconElement.textureId = item.iconName;
                    iconElement.rect = SDL_Rect{10, 0, iconW, iconH};  // Y는 updateItemPositions에서 설정
                    iconElement.visible = true;
                    iconElement.clickable = false;
                    
                    std::string iconId = uiManager->addUIAndGetId(iconElement);
                    uiManager->setParent(iconId, uiElementId);
                    visible.iconElementId = iconId;
                    
                    // 텍스트 위치 (아이콘 오른쪽)
                    bgElement->rect.x = 10 + iconW + iconTextSpacing;
                } else {
                    // 아이콘이 없으면 왼쪽 정렬
                    bgElement->rect.x = 10;
                }
            } else {
                bgElement->rect.x = 10;
            }
            
            uiManager->setParent(bgElementId, uiElementId);
            visible.textWidget = std::move(textWidget);
            break;
        }
        
        case ListItemType::RIGHT_ICON_TEXT: {
            // 오른쪽 아이콘 + 텍스트
            int textMaxWidth = maxWidth - iconSize - iconTextSpacing - 20;
            if (textMaxWidth <= 0) {
                textMaxWidth = maxWidth - 20;
            }
            
            auto textWidget = std::make_unique<BackgroundTextWidget>(
                uiManager, resourceManager, renderer, textRenderer,
                item.text, fontSize, textColor, backgroundColor, textMaxWidth, -1
            );
            
            // 배경 이미지 설정
            if (!backgroundImageName.empty()) {
                textWidget->setBackgroundImage(backgroundImageName, useNinePatch, useThreePatch);
            }
            
            std::string bgElementId = textWidget->getBackgroundElementId();
            auto* bgElement = uiManager->findElementByName(bgElementId);
            if (!bgElement) break;
            
            int textW = bgElement->rect.w;
            int textH = bgElement->rect.h;
            
            // 아이콘 생성
            if (!item.iconName.empty()) {
                SDL_Texture* iconTexture = resourceManager->getTexture(item.iconName);
                if (iconTexture) {
                    int origW, origH;
                    SDL_QueryTexture(iconTexture, nullptr, nullptr, &origW, &origH);
                    float scale = std::min(static_cast<float>(iconSize) / origW, 
                                          static_cast<float>(iconSize) / origH);
                    int iconW = static_cast<int>(origW * scale);
                    int iconH = static_cast<int>(origH * scale);
                    
                    UIElement iconElement;
                    iconElement.name = "";
                    iconElement.texture = iconTexture;
                    iconElement.textureId = item.iconName;
                    iconElement.rect = SDL_Rect{maxWidth - iconW - 10, 0, iconW, iconH};  // Y는 updateItemPositions에서 설정
                    iconElement.visible = true;
                    iconElement.clickable = false;
                    
                    std::string iconId = uiManager->addUIAndGetId(iconElement);
                    uiManager->setParent(iconId, uiElementId);
                    visible.iconElementId = iconId;
                    
                    // 텍스트 위치 (아이콘 왼쪽)
                    bgElement->rect.x = maxWidth - iconW - iconTextSpacing - textW - 10;
                } else {
                    // 아이콘이 없으면 오른쪽 정렬
                    bgElement->rect.x = maxWidth - textW - 10;
                }
            } else {
                bgElement->rect.x = maxWidth - textW - 10;
            }
            
            uiManager->setParent(bgElementId, uiElementId);
            visible.textWidget = std::move(textWidget);
            break;
        }
        
        case ListItemType::CENTER_IMAGE: {
            // 가운데 이미지
            if (!item.imageName.empty()) {
                SDL_Texture* imageTexture = resourceManager->getTexture(item.imageName);
                if (imageTexture) {
                    // 이미지 크기: width는 위젯의 절반, height는 width의 절반
                    int imgW = maxWidth / 2;
                    int imgH = imgW / 2;
                    
                    UIElement imageElement;
                    imageElement.name = "";
                    imageElement.texture = imageTexture;
                    imageElement.textureId = item.imageName;
                    imageElement.rect = SDL_Rect{(maxWidth - imgW) / 2, 0, imgW, imgH};  // Y는 updateItemPositions에서 설정
                    imageElement.visible = true;
                    imageElement.clickable = false;
                    
                    std::string imageId = uiManager->addUIAndGetId(imageElement);
                    uiManager->setParent(imageId, uiElementId);
                    visible.imageElementId = imageId;
                }
            }
            break;
        }
    }
    
    visibleItems.push_back(std::move(visible));
}

void MultiTypeListWidget::removeItemElement(int index) {
    // visibleItems에서 해당 인덱스 찾기
    for (auto it = visibleItems.begin(); it != visibleItems.end(); ++it) {
        if (it->itemIndex == index) {
            // BackgroundTextWidget 제거 (소멸자에서 UIElement 자동 제거)
            if (it->textWidget) {
                it->textWidget.reset();
            }
            
            // 아이콘 UIElement 제거
            if (!it->iconElementId.empty()) {
                uiManager->removeUI(it->iconElementId);
            }
            
            // 이미지 UIElement 제거
            if (!it->imageElementId.empty()) {
                uiManager->removeUI(it->imageElementId);
            }
            
            // visibleItems.erase(it)는 updateVisibleItems()에서 처리
            // 여기서는 UIElement만 제거하고 visibleItems는 그대로 둠
            break;
        }
    }
}

void MultiTypeListWidget::updateItemPositions() {
    for (auto& visible : visibleItems) {
        // 인덱스 범위 체크
        if (visible.itemIndex < 0 || visible.itemIndex >= static_cast<int>(items.size())) {
            continue;
        }
        
        int index = visible.itemIndex;
        int baseY = index * itemHeight - scrollOffset;
        
        // 텍스트 위젯 위치
        if (visible.textWidget) {
            auto* bgElement = uiManager->findElementByName(
                visible.textWidget->getBackgroundElementId()
            );
            if (bgElement) {
                int textH = bgElement->rect.h;
                bgElement->rect.y = baseY + (itemHeight - textH) / 2;  // 수직 중앙 정렬
            }
        }
        
        // 아이콘 위치
        if (!visible.iconElementId.empty()) {
            auto* iconElement = uiManager->findElementByName(visible.iconElementId);
            if (iconElement) {
                int iconH = iconElement->rect.h;
                iconElement->rect.y = baseY + (itemHeight - iconH) / 2;  // 수직 중앙 정렬
            }
        }
        
        // 이미지 위치
        if (!visible.imageElementId.empty()) {
            auto* imageElement = uiManager->findElementByName(visible.imageElementId);
            if (imageElement) {
                int imgH = imageElement->rect.h;
                imageElement->rect.y = baseY + (itemHeight - imgH) / 2;  // 수직 중앙 정렬
            }
        }
    }
}

void MultiTypeListWidget::clearVisibleItems() {
    for (auto& visible : visibleItems) {
        if (visible.textWidget) {
            visible.textWidget.reset();
        }
        if (!visible.iconElementId.empty()) {
            uiManager->removeUI(visible.iconElementId);
        }
        if (!visible.imageElementId.empty()) {
            uiManager->removeUI(visible.imageElementId);
        }
    }
    visibleItems.clear();
}

