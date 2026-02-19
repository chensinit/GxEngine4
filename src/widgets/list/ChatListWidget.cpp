#include "ChatListWidget.h"
#include "../basic/BackgroundTextWidget.h"
#include "../../ui/uiManager.h"
#include "../../scene.h"  // MOUSE_CLICK_EVENT 접근용
#include "../../rendering/TextRenderer.h"
#include "../../resource/resourceManager.h"
#include "../../utils/logger.h"
#include <algorithm>
#include <sstream>

ChatListWidget::ChatListWidget(UiManager* uiMgr, ResourceManager* resMgr,
                               SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                               int itemH, int size, SDL_Color color, int iconSz, int spacing,
                               const SDL_Rect& rect, float scale, float rotation,
                               float alpha, bool visible, bool clickable)
    : UIWidget(uiMgr, resMgr),
      itemHeight(itemH),
      fontSize(size),
      textColor(color),
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
    element.name = "";  // 빈 이름으로 자동 ID 생성
    element.texture = nullptr;  // ChatListWidget은 직접 렌더링
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

ChatListWidget::~ChatListWidget() {
    clearMessageElements();
}


void ChatListWidget::setMessages(const std::vector<ChatMessage>& newMessages) {
    clearMessageElements();
    messages = newMessages;
    updateMessageElements();
    // 스크롤 오프셋이 범위를 벗어나지 않도록 조정
    int maxScroll = static_cast<int>(messages.size()) * itemHeight - maxVisibleItems * itemHeight;
    if (maxScroll < 0) maxScroll = 0;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;
    if (scrollOffset < 0) scrollOffset = 0;
    updateMessagePositions();
}

void ChatListWidget::addMessage(const std::string& text, const std::string& iconName, ChatAlignment alignment) {
    messages.push_back(ChatMessage(text, iconName, alignment));
    // 전체 메시지 요소 재생성
    updateMessageElements();
}

void ChatListWidget::clearMessages() {
    clearMessageElements();
    messages.clear();
    scrollOffset = 0;
}

void ChatListWidget::setItemHeight(int height) {
    itemHeight = height;
    updateMessageElements();
}

void ChatListWidget::setFontSize(int size) {
    if (fontSize == size) return;
    fontSize = size;
    updateMessageElements();
}

void ChatListWidget::setTextColor(SDL_Color color) {
    if (textColor.r == color.r && textColor.g == color.g && 
        textColor.b == color.b && textColor.a == color.a) {
        return;
    }
    textColor = color;
    updateMessageElements();
}

void ChatListWidget::setIconSize(int size) {
    if (iconSize == size) return;
    iconSize = size;
    updateMessageElements();
}

void ChatListWidget::setIconTextSpacing(int spacing) {
    if (iconTextSpacing == spacing) return;
    iconTextSpacing = spacing;
    updateMessageElements();
}

void ChatListWidget::setBackgroundImage(const std::string& imageName, bool useNinePatchParam, bool useThreePatchParam) {
    if (backgroundImageName == imageName && useNinePatch == useNinePatchParam && useThreePatch == useThreePatchParam) return;
    backgroundImageName = imageName;
    useNinePatch = useNinePatchParam;
    useThreePatch = useThreePatchParam;
    updateMessageElements();
}

void ChatListWidget::setScrollOffset(int offset) {
    scrollOffset = offset;
    // 범위 제한
    auto* parentElement = uiManager->findElementByName(uiElementId);
    if (!parentElement) return;
    int widgetH = parentElement->rect.h;
    int maxScroll = static_cast<int>(messages.size()) * itemHeight - widgetH;
    if (maxScroll < 0) maxScroll = 0;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;
    if (scrollOffset < 0) scrollOffset = 0;
    updateMessagePositions();
}

void ChatListWidget::scrollToTop() {
    scrollOffset = 0;
    updateMessagePositions();
}

void ChatListWidget::scrollToBottom() {
    auto* parentElement = uiManager->findElementByName(uiElementId);
    if (!parentElement) return;
    int widgetH = parentElement->rect.h;
    int maxScroll = static_cast<int>(messages.size()) * itemHeight - widgetH;
    if (maxScroll < 0) maxScroll = 0;
    scrollOffset = maxScroll;
    updateMessagePositions();
}

void ChatListWidget::handleEvent(const SDL_Event& event) {
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

void ChatListWidget::update(float deltaTime) {
    // 스크롤 위치 업데이트는 필요시 handleEvent에서 처리
    // 여기서는 추가 업데이트 로직이 필요하면 추가
}


void ChatListWidget::updateMessageElements() {
    clearMessageElements();
    
    // 위젯 크기 가져오기
    auto* parentElement = uiManager->findElementByName(uiElementId);
    if (!parentElement) {
        Log::error("[ChatListWidget] updateMessageElements: Parent element not found: ", uiElementId);
        return;
    }
    int maxWidth = parentElement->rect.w;
    if (maxWidth <= 0) {
        Log::error("[ChatListWidget] updateMessageElements: Invalid width: ", maxWidth);
        return;
    }
    
    // 각 메시지에 대해 BackgroundTextWidget과 아이콘 UIElement 생성
    for (size_t i = 0; i < messages.size(); i++) {
        const ChatMessage& msg = messages[i];
        int baseY = static_cast<int>(i) * itemHeight;
        
        // 텍스트 영역 계산 (아이콘 공간 제외)
        int textMaxWidth = maxWidth - iconSize - iconTextSpacing - 20;
        if (textMaxWidth <= 0) {
            textMaxWidth = maxWidth - 20;  // 최소한의 여백
        }
        
        // BackgroundTextWidget 생성 (노란색 배경, 검정색 텍스트)
        SDL_Color bgColor = {255, 255, 0, 255};  // 노란색 배경
        SDL_Color txtColor = {0, 0, 0, 255};     // 검정색 텍스트
        auto bgTextWidget = std::make_unique<BackgroundTextWidget>(
            uiManager, resourceManager, renderer, textRenderer,
            msg.text, fontSize, txtColor, bgColor, textMaxWidth, -1
        );
        
        // 배경 이미지 설정
        if (!backgroundImageName.empty()) {
            bgTextWidget->setBackgroundImage(backgroundImageName, useNinePatch, useThreePatch);
        }
        
        // 배경 UIElement ID 가져오기
        std::string bgElementId = bgTextWidget->getBackgroundElementId();
        auto* bgElement = uiManager->findElementByName(bgElementId);
        if (!bgElement) {
            Log::error("[ChatListWidget] updateMessageElements: Background element not found");
            continue;
        }
        
        int textW = bgElement->rect.w;
        int textH = bgElement->rect.h;
        
        // 아이콘 텍스처 가져오기 및 크기 계산
        SDL_Texture* iconTexture = nullptr;
        int iconW = 0, iconH = 0;
        int iconX = 0, iconY = 0;
        
        if (!msg.iconName.empty()) {
            iconTexture = resourceManager->getTexture(msg.iconName);
            if (iconTexture) {
                int origW, origH;
                SDL_QueryTexture(iconTexture, nullptr, nullptr, &origW, &origH);
                // 비율 유지하면서 크기 조정
                float scale = std::min(static_cast<float>(iconSize) / origW, static_cast<float>(iconSize) / origH);
                iconW = static_cast<int>(origW * scale);
                iconH = static_cast<int>(origH * scale);
            }
        }
        
        // 위치 계산
        int textX = 0, textY = 0;
        if (msg.alignment == ChatAlignment::LEFT) {
            // 왼쪽 정렬: 아이콘 왼쪽, 텍스트 아이콘 오른쪽
            if (iconTexture && iconW > 0) {
                iconX = 10;  // 왼쪽 여백
                iconY = baseY + (itemHeight - iconH) / 2;  // 수직 중앙 정렬
                textX = iconX + iconW + iconTextSpacing;
            } else {
                textX = 10;  // 아이콘이 없으면 왼쪽 여백만
            }
            textY = baseY + (itemHeight - textH) / 2;  // 수직 중앙 정렬
        } else {
            // 오른쪽 정렬: 텍스트 왼쪽, 아이콘 텍스트 오른쪽
            if (iconTexture && iconW > 0) {
                textX = maxWidth - iconW - iconTextSpacing - textW - 10;  // 오른쪽 여백 고려
                iconX = textX + textW + iconTextSpacing;
                iconY = baseY + (itemHeight - iconH) / 2;  // 수직 중앙 정렬
            } else {
                textX = maxWidth - textW - 10;  // 아이콘이 없으면 오른쪽 정렬만
            }
            textY = baseY + (itemHeight - textH) / 2;  // 수직 중앙 정렬
        }
        
        // 배경 위젯의 위치 설정
        bgElement->rect.x = textX;
        bgElement->rect.y = textY;
        
        // 배경 위젯을 ChatListWidget의 자식으로 설정
        uiManager->setParent(bgElementId, uiElementId);
        
        // BackgroundTextWidget 저장
        messageTextWidgets.push_back(std::move(bgTextWidget));
        
        // 아이콘 UIElement 생성 (아이콘이 있는 경우)
        if (iconTexture && iconW > 0 && iconH > 0) {
            UIElement iconElement;
            iconElement.name = "";  // 빈 문자열로 전달하면 자동 ID 생성
            iconElement.texture = iconTexture;
            iconElement.textureId = msg.iconName;  // ResourceManager에 등록된 이름 사용
            iconElement.rect = SDL_Rect{iconX, iconY, iconW, iconH};
            iconElement.visible = true;
            iconElement.clickable = false;
            iconElement.parentName = uiElementId;  // ChatListWidget을 부모로 설정
            
            std::string iconElementId = uiManager->addUIAndGetId(iconElement);
            uiManager->setParent(iconElementId, uiElementId);
            messageIconElementNames.push_back(iconElementId);
        }
    }
    
    updateMessagePositions();
}

void ChatListWidget::updateMessagePositions() {
    // 각 메시지의 BackgroundTextWidget과 아이콘 UIElement의 Y 위치를 스크롤 오프셋에 맞게 조정
    size_t iconIndex = 0;
    for (size_t i = 0; i < messages.size(); i++) {
        int baseY = static_cast<int>(i) * itemHeight - scrollOffset;
        
        // BackgroundTextWidget 위치 업데이트
        if (i < messageTextWidgets.size() && messageTextWidgets[i]) {
            std::string bgElementId = messageTextWidgets[i]->getBackgroundElementId();
            auto* bgElement = uiManager->findElementByName(bgElementId);
            if (bgElement) {
                int textH = bgElement->rect.h;
                bgElement->rect.y = baseY + (itemHeight - textH) / 2;
            }
        }
        
        // 아이콘 요소 위치 업데이트 (아이콘이 있는 경우)
        if (!messages[i].iconName.empty() && iconIndex < messageIconElementNames.size()) {
            UIElement* iconElement = uiManager->findElementByName(messageIconElementNames[iconIndex]);
            if (iconElement) {
                iconElement->rect.y = baseY + (itemHeight - iconElement->rect.h) / 2;
            }
            iconIndex++;
        }
    }
}

void ChatListWidget::clearMessageElements() {
    if (!uiManager || !resourceManager) return;  // 안전 체크
    
    // BackgroundTextWidget들 제거 (소멸자에서 자동으로 텍스처 해제 및 UIElement 제거)
    messageTextWidgets.clear();
    
    // 아이콘 UIElement 제거
    for (const auto& elementName : messageIconElementNames) {
        UIElement* element = uiManager->findElementByName(elementName);
        if (element) {
            // 아이콘은 ResourceManager에 정적으로 등록되어 있으므로 텍스처 해제하지 않음
            // UIElement만 제거
            uiManager->removeUI(elementName);
        }
    }
    
    messageIconElementNames.clear();
}
