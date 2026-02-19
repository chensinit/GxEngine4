#include "UpgradeListWidget.h"
#include "../basic/BackgroundTextWidget.h"
#include "../basic/ButtonWidget.h"
#include "../WidgetManager.h"
#include "../../ui/uiManager.h"
#include "../../animation/AnimationManager.h"
#include "../../rendering/TextRenderer.h"
#include "../../resource/resourceManager.h"
#include "../../utils/logger.h"
#include <algorithm>
#include <set>

namespace {
    const int ITEM_MARGIN = 6;       // 리스트 항목 간 간격 (px)
    const int PAD_LEFT = 20;         // 왼쪽 마진 (아이콘 왼쪽)
    const int GAP_ICON_TEXT = 12;    // 아이콘과 텍스트 사이
    const int GAP_TEXT_BUTTON = 12;  // 텍스트와 버튼 사이
    const int PAD_RIGHT = 10;        // 오른쪽 마진 (버튼 오른쪽)
}

UpgradeListWidget::UpgradeListWidget(UiManager* uiMgr, ResourceManager* resMgr,
                             SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                             int itemH, int iconSz, int iconMarg,
                             int titleSize, int descSize,
                             int btnW, int btnH, int btnFontSize,
                             SDL_Color titleClr, SDL_Color descClr, SDL_Color btnTxtClr,
                             const SDL_Rect& rect,
                             float scale, float rotation, float alpha,
                             bool visible, bool clickable)
    : UIWidget(uiMgr, resMgr),
      renderer(sdlRenderer),
      textRenderer(txtRenderer),
      itemHeight(itemH),
      iconSize(iconSz),
      iconMargin(iconMarg),
      titleFontSize(titleSize),
      descFontSize(descSize),
      buttonWidth(btnW),
      buttonHeight(btnH),
      buttonFontSize(btnFontSize),
      titleColor(titleClr),
      descColor(descClr),
      buttonTextColor(btnTxtClr),
      itemBackgroundImage(""),
      useItemBackgroundNinePatch(false),
      useItemBackgroundThreePatch(false),
      useButtonNinePatch(false),
      useButtonThreePatch(false),
      scrollOffset(0),
      maxVisibleItems(0),
      isDragging(false),
      wasDraggableOnMouseDown(false),
      lastMouseY(0),
      dragStartY(0),
      uiElementId(""),
      widgetManager(nullptr),
      animationManager(nullptr) {
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
    
    if (rect.h > 0) {
        int itemSpacing = itemHeight + ITEM_MARGIN;
        maxVisibleItems = rect.h / itemSpacing + 2;
    }
}

UpgradeListWidget::~UpgradeListWidget() {
    // WidgetManager가 widgets를 파괴하는 중에 호출되므로 removeWidget 호출 금지
    clearVisibleItems(false);
}

void UpgradeListWidget::addItem(const UpgradeItem& item) {
    items.push_back(item);
    updateVisibleItems();
}

void UpgradeListWidget::addItem(const std::string& iconName, const std::string& title,
                            const std::string& desc, const std::string& buttonImage,
                            const std::string& buttonText) {
    addItem(UpgradeItem(iconName, title, desc, buttonImage, buttonText));
}

void UpgradeListWidget::clearItems() {
    clearVisibleItems();
    items.clear();
    scrollOffset = 0;
}

void UpgradeListWidget::setItemHeight(int height) {
    if (itemHeight == height) return;
    itemHeight = height;
    auto* el = uiManager->findElementByName(uiElementId);
    if (el && el->rect.h > 0) {
        int itemSpacing = itemHeight + ITEM_MARGIN;
        maxVisibleItems = el->rect.h / itemSpacing + 2;
    }
    updateVisibleItems();
}

void UpgradeListWidget::setIconSize(int size) {
    if (iconSize == size) return;
    iconSize = size;
    updateVisibleItems();
}

void UpgradeListWidget::setIconMargin(int margin) {
    if (iconMargin == margin) return;
    iconMargin = margin;
    updateVisibleItems();
}

void UpgradeListWidget::setTitleFontSize(int size) {
    if (titleFontSize == size) return;
    titleFontSize = size;
    updateVisibleItems();
}

void UpgradeListWidget::setDescFontSize(int size) {
    if (descFontSize == size) return;
    descFontSize = size;
    updateVisibleItems();
}

void UpgradeListWidget::setButtonSize(int w, int h) {
    if (buttonWidth == w && buttonHeight == h) return;
    buttonWidth = w;
    buttonHeight = h;
    updateVisibleItems();
}

void UpgradeListWidget::setButtonFontSize(int size) {
    if (buttonFontSize == size) return;
    buttonFontSize = size;
    updateVisibleItems();
}

void UpgradeListWidget::setTitleColor(SDL_Color color) {
    if (titleColor.r == color.r && titleColor.g == color.g &&
        titleColor.b == color.b && titleColor.a == color.a) return;
    titleColor = color;
    updateVisibleItems();
}

void UpgradeListWidget::setDescColor(SDL_Color color) {
    if (descColor.r == color.r && descColor.g == color.g &&
        descColor.b == color.b && descColor.a == color.a) return;
    descColor = color;
    updateVisibleItems();
}

void UpgradeListWidget::setButtonTextColor(SDL_Color color) {
    if (buttonTextColor.r == color.r && buttonTextColor.g == color.g &&
        buttonTextColor.b == color.b && buttonTextColor.a == color.a) return;
    buttonTextColor = color;
    updateVisibleItems();
}

void UpgradeListWidget::setItemBackgroundImage(const std::string& imageName, bool useNinePatch, bool useThreePatch) {
    if (itemBackgroundImage == imageName && useItemBackgroundNinePatch == useNinePatch && useItemBackgroundThreePatch == useThreePatch) return;
    itemBackgroundImage = imageName;
    useItemBackgroundNinePatch = useNinePatch;
    useItemBackgroundThreePatch = useThreePatch;
    clearVisibleItems();
    updateVisibleItems();
}

void UpgradeListWidget::setScrollOffset(int offset) {
    scrollOffset = offset;
    auto* el = uiManager->findElementByName(uiElementId);
    if (!el) return;
    int widgetH = el->rect.h;
    int itemSpacing = itemHeight + ITEM_MARGIN;
    int maxScroll = static_cast<int>(items.size()) * itemSpacing - widgetH;
    if (maxScroll < 0) maxScroll = 0;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;
    if (scrollOffset < 0) scrollOffset = 0;
    updateItemPositions();
    updateVisibleItems();
}

void UpgradeListWidget::scrollToTop() {
    scrollOffset = 0;
    updateItemPositions();
    updateVisibleItems();
}

void UpgradeListWidget::scrollToBottom() {
    auto* el = uiManager->findElementByName(uiElementId);
    if (!el) return;
    int widgetH = el->rect.h;
    int itemSpacing = itemHeight + ITEM_MARGIN;
    int maxScroll = static_cast<int>(items.size()) * itemSpacing - widgetH;
    if (maxScroll < 0) maxScroll = 0;
    scrollOffset = maxScroll;
    updateItemPositions();
    updateVisibleItems();
}

void UpgradeListWidget::handleEvent(const SDL_Event& event) {
    auto* el = uiManager->findElementByName(uiElementId);
    if (!el) return;
    
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int x = event.button.x;
        int y = event.button.y;
        wasDraggableOnMouseDown = isPointInside(x, y);
        if (!wasDraggableOnMouseDown) return;
        // 버튼 위에서 눌렸으면 드래그 시작 안 함 (ButtonWidget이 처리)
        if (widgetManager) {
            for (auto& vi : visibleItems) {
                if (vi.buttonWidgetName.empty()) continue;
                auto* btnWidget = widgetManager->getWidget(vi.buttonWidgetName);
                if (!btnWidget) continue;
                if (btnWidget->isPointInside(x, y)) {
                    return;
                }
            }
        }
        isDragging = true;
        lastMouseY = event.button.y;
        dragStartY = event.button.y;
    }
    else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        if (wasDraggableOnMouseDown) {
            isDragging = false;
            wasDraggableOnMouseDown = false;
        }
    }
    else if (event.type == SDL_MOUSEMOTION) {
        if (isDragging) {
            int mouseY = event.motion.y;
            int deltaY = mouseY - lastMouseY;
            setScrollOffset(scrollOffset - deltaY);
            lastMouseY = mouseY;
        }
    }
    // MOUSE_CLICK_EVENT는 ButtonWidget이 자체적으로 처리
}

void UpgradeListWidget::update(float deltaTime) {}

void UpgradeListWidget::updateVisibleItems() {
    if (items.empty()) {
        clearVisibleItems();
        return;
    }
    
    auto* el = uiManager->findElementByName(uiElementId);
    if (!el) return;
    
    int viewportH = el->rect.h;
    if (viewportH <= 0) return;
    
    int itemSpacing = itemHeight + ITEM_MARGIN;
    int startIndex = scrollOffset / itemSpacing;
    if (startIndex < 0) startIndex = 0;
    if (startIndex >= static_cast<int>(items.size())) {
        clearVisibleItems();
        return;
    }
    
    int endIndex = startIndex + (viewportH / itemSpacing) + 2;
    endIndex = std::min(endIndex, static_cast<int>(items.size()) - 1);
    if (endIndex < startIndex) {
        clearVisibleItems();
        return;
    }
    
    std::set<int> currentVisible;
    for (int i = startIndex; i <= endIndex; i++) currentVisible.insert(i);
    
    for (auto it = visibleItems.begin(); it != visibleItems.end();) {
        if (currentVisible.find(it->itemIndex) == currentVisible.end()) {
            if (it->titleWidget) it->titleWidget.reset();
            if (it->descWidget) it->descWidget.reset();
            if (!it->rowBgElementId.empty()) {
                uiManager->removeUI(it->rowBgElementId);
                if (!it->rowBgTextureId.empty()) {
                    resourceManager->unregisterTexture(it->rowBgTextureId);
                }
            }
            if (!it->iconElementId.empty()) uiManager->removeUI(it->iconElementId);
            if (!it->buttonWidgetName.empty() && widgetManager) {
                widgetManager->removeWidget(it->buttonWidgetName);
            }
            it = visibleItems.erase(it);
        } else {
            ++it;
        }
    }
    
    for (int i = startIndex; i <= endIndex; i++) {
        bool exists = false;
        for (const auto& v : visibleItems) {
            if (v.itemIndex == i) { exists = true; break; }
        }
        if (!exists) createItemElement(i);
    }
    
    updateItemPositions();
}

void UpgradeListWidget::createItemElement(int index) {
    if (index < 0 || index >= static_cast<int>(items.size())) return;
    
    const UpgradeItem& item = items[index];
    auto* parentEl = uiManager->findElementByName(uiElementId);
    if (!parentEl) return;
    
    int maxW = parentEl->rect.w;
    if (maxW <= 0) return;
    
    int iconX = PAD_LEFT;
    int centerX = iconX + iconSize + GAP_ICON_TEXT;
    int centerW = maxW - centerX - buttonWidth - GAP_TEXT_BUTTON - PAD_RIGHT;
    if (centerW < 20) centerW = 20;
    
    VisibleItem vi;
    vi.itemIndex = index;
    
    if (!itemBackgroundImage.empty()) {
        SDL_Texture* bgTex = nullptr;
        if (useItemBackgroundNinePatch) {
            SDL_Texture* baseTex = resourceManager->getTexture(itemBackgroundImage);
            if (baseTex) {
                bgTex = resourceManager->createNinePatchTexture(baseTex, maxW, itemHeight);
                if (bgTex) {
                    vi.rowBgTextureId = resourceManager->registerTexture(bgTex);
                }
            }
        } else if (useItemBackgroundThreePatch) {
            SDL_Texture* baseTex = resourceManager->getTexture(itemBackgroundImage);
            if (baseTex) {
                bgTex = resourceManager->createThreePatchTexture(baseTex, maxW, itemHeight);
                if (bgTex) {
                    vi.rowBgTextureId = resourceManager->registerTexture(bgTex);
                }
            }
        } else {
            bgTex = resourceManager->getTexture(itemBackgroundImage);
        }
        if (bgTex) {
            UIElement bgEl;
            bgEl.name = "";
            bgEl.texture = bgTex;
            bgEl.textureId = vi.rowBgTextureId.empty() ? itemBackgroundImage : vi.rowBgTextureId;
            bgEl.rect = SDL_Rect{0, 0, maxW, itemHeight};
            bgEl.visible = true;
            bgEl.clickable = false;
            vi.rowBgElementId = uiManager->addUIAndGetId(bgEl);
            uiManager->setParent(vi.rowBgElementId, uiElementId);
        }
    }
    
    if (!item.iconName.empty()) {
        SDL_Texture* iconTex = resourceManager->getTexture(item.iconName);
        if (iconTex) {
            int iw, ih;
            SDL_QueryTexture(iconTex, nullptr, nullptr, &iw, &ih);
            float scale = std::min(static_cast<float>(iconSize) / iw,
                                  static_cast<float>(iconSize) / ih);
            int iconW = static_cast<int>(iw * scale);
            int iconH = static_cast<int>(ih * scale);
            
            UIElement iconEl;
            iconEl.name = "";
            iconEl.texture = iconTex;
            iconEl.textureId = item.iconName;
            iconEl.rect = SDL_Rect{iconX, 0, iconW, iconH};
            iconEl.visible = true;
            iconEl.clickable = false;
            
            vi.iconElementId = uiManager->addUIAndGetId(iconEl);
            uiManager->setParent(vi.iconElementId, uiElementId);
        }
    }
    
    SDL_Color transparent = {0, 0, 0, 0};
    
    if (!item.titleText.empty()) {
        int titleMaxH = itemHeight / 2;
        auto titleW = std::make_unique<BackgroundTextWidget>(
            uiManager, resourceManager, renderer, textRenderer,
            item.titleText, titleFontSize, titleColor, transparent,
            centerW, titleMaxH
        );
        titleW->setBackgroundColor(transparent);
        
        std::string bgId = titleW->getBackgroundElementId();
        auto* bgEl = uiManager->findElementByName(bgId);
        if (bgEl) {
            bgEl->rect.x = centerX;
            bgEl->rect.y = 4;
        }
        uiManager->setParent(bgId, uiElementId);
        vi.titleWidget = std::move(titleW);
    }
    
    if (!item.descText.empty()) {
        int descMaxH = itemHeight - itemHeight / 2 - 8;
        if (descMaxH < 10) descMaxH = itemHeight - 20;
        auto descW = std::make_unique<BackgroundTextWidget>(
            uiManager, resourceManager, renderer, textRenderer,
            item.descText, descFontSize, descColor, transparent,
            centerW, descMaxH
        );
        descW->setBackgroundColor(transparent);
        
        std::string bgId = descW->getBackgroundElementId();
        auto* bgEl = uiManager->findElementByName(bgId);
        if (bgEl) {
            bgEl->rect.x = centerX;
            bgEl->rect.y = itemHeight / 2 + 2;
        }
        uiManager->setParent(bgId, uiElementId);
        vi.descWidget = std::move(descW);
    }
    
    int btnX = maxW - PAD_RIGHT - GAP_TEXT_BUTTON - buttonWidth;
    if (widgetManager && animationManager && !item.buttonImage.empty()) {
        std::string btnName = uiElementId + "_btn_" + std::to_string(index);
        SDL_Rect btnRect = {btnX, 0, buttonWidth, buttonHeight};
        auto btnWidget = std::make_unique<ButtonWidget>(
            uiManager, resourceManager, renderer, textRenderer,
            item.buttonImage, item.buttonImage,
            btnRect, animationManager
        );
        btnWidget->setText(item.buttonText);
        btnWidget->setFontSize(buttonFontSize);
        btnWidget->setTextColor(buttonTextColor);
        if (useButtonNinePatch || useButtonThreePatch) {
            btnWidget->setNormalImage(item.buttonImage, useButtonNinePatch, useButtonThreePatch);
        }
        int itemIdx = index;
        btnWidget->setOnClick([this, itemIdx]() {
            if (onItemButtonClick) onItemButtonClick(itemIdx);
        });
        uiManager->setParent(btnWidget->getUIElementIdentifier(), uiElementId);
        widgetManager->addWidget(btnName, std::move(btnWidget));
        vi.buttonWidgetName = btnName;
    }
    
    visibleItems.push_back(std::move(vi));
}

void UpgradeListWidget::updateItemPositions() {
    auto* parentEl = uiManager->findElementByName(uiElementId);
    if (!parentEl) return;
    int maxW = parentEl->rect.w;
    
    for (auto& vi : visibleItems) {
        if (vi.itemIndex < 0 || vi.itemIndex >= static_cast<int>(items.size()))
            continue;
        
        int itemSpacing = itemHeight + ITEM_MARGIN;
        int baseY = vi.itemIndex * itemSpacing - scrollOffset;
        
        if (!vi.rowBgElementId.empty()) {
            auto* el = uiManager->findElementByName(vi.rowBgElementId);
            if (el) {
                el->rect.y = baseY;
            }
        }
        
        if (!vi.iconElementId.empty()) {
            auto* el = uiManager->findElementByName(vi.iconElementId);
            if (el) {
                int ih = el->rect.h;
                el->rect.y = baseY + (itemHeight - ih) / 2;
            }
        }
        
        if (vi.titleWidget) {
            auto* bgEl = uiManager->findElementByName(vi.titleWidget->getBackgroundElementId());
            if (bgEl) {
                int th = bgEl->rect.h;
                bgEl->rect.y = baseY + 4;
            }
        }
        
        if (vi.descWidget) {
            auto* bgEl = uiManager->findElementByName(vi.descWidget->getBackgroundElementId());
            if (bgEl) {
                bgEl->rect.y = baseY + itemHeight / 2 + 2;
            }
        }
        
        if (!vi.buttonWidgetName.empty() && widgetManager) {
            auto* btnWidget = widgetManager->getWidget(vi.buttonWidgetName);
            if (btnWidget) {
                auto* btn = dynamic_cast<ButtonWidget*>(btnWidget);
                if (btn) {
                    int btnX = maxW - PAD_RIGHT - GAP_TEXT_BUTTON - buttonWidth;
                    int bh = buttonHeight;
                    btn->setRect(btnX, baseY + (itemHeight - bh) / 2, buttonWidth, bh);
                }
            }
        }
    }
}

void UpgradeListWidget::clearVisibleItems(bool removeChildWidgets) {
    for (auto& vi : visibleItems) {
        if (vi.titleWidget) vi.titleWidget.reset();
        if (vi.descWidget) vi.descWidget.reset();
        if (!vi.rowBgElementId.empty()) {
            uiManager->removeUI(vi.rowBgElementId);
            if (!vi.rowBgTextureId.empty()) {
                resourceManager->unregisterTexture(vi.rowBgTextureId);
            }
        }
        if (!vi.iconElementId.empty()) uiManager->removeUI(vi.iconElementId);
        if (removeChildWidgets && !vi.buttonWidgetName.empty() && widgetManager) {
            widgetManager->removeWidget(vi.buttonWidgetName);
        }
    }
    visibleItems.clear();
}
