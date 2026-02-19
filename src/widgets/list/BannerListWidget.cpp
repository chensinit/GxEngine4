#include "BannerListWidget.h"
#include "../../ui/uiManager.h"
#include "../../scene.h"
#include "../../rendering/TextRenderer.h"
#include "../../rendering/ImageRenderer.h"
#include "../../resource/resourceManager.h"
#include "../../utils/logger.h"
#include <algorithm>
#include <set>

BannerListWidget::BannerListWidget(UiManager* uiMgr, ResourceManager* resMgr,
                                  SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                                  int itemH, const std::string& bgImage, SDL_Color bgColor,
                                  int fontSz, SDL_Color txtColor,
                                  BannerTextAlignH alignH, BannerTextAlignV alignV,
                                  const SDL_Rect& rect,
                                  float scale, float rotation, float alpha,
                                  bool visible, bool clickable)
    : UIWidget(uiMgr, resMgr),
      renderer(sdlRenderer),
      textRenderer(txtRenderer),
      itemHeight(itemH),
      itemMargin(0),
      backgroundImageName(bgImage),
      backgroundColor(bgColor),
      fontSize(fontSz),
      textColor(txtColor),
      textAlignH(alignH),
      textAlignV(alignV),
      scrollOffset(0),
      maxVisibleItems(0),
      isDragging(false),
      wasDraggableOnMouseDown(false),
      lastMouseY(0),
      dragStartY(0),
      uiElementId("") {
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
        int slotH = itemHeight + itemMargin;
        maxVisibleItems = (slotH > 0) ? (rect.h / slotH + 2) : 2;
    }
}

BannerListWidget::~BannerListWidget() {
    clearVisibleItems();
}

void BannerListWidget::addItem(const std::string& title) {
    items.push_back(BannerItem{title, ""});
    updateVisibleItems();
}

void BannerListWidget::addItem(const std::string& title, const std::string& imageName) {
    items.push_back(BannerItem{title, imageName});
    updateVisibleItems();
}

void BannerListWidget::clearItems() {
    clearVisibleItems();
    items.clear();
    scrollOffset = 0;
}

void BannerListWidget::setItemHeight(int height) {
    if (itemHeight == height) return;
    itemHeight = height;
    auto* el = uiManager->findElementByName(uiElementId);
    if (el && el->rect.h > 0) {
        int slotH = itemHeight + itemMargin;
        maxVisibleItems = (slotH > 0) ? (el->rect.h / slotH + 2) : 2;
    }
    updateVisibleItems();
}

void BannerListWidget::setItemMargin(int margin) {
    if (itemMargin == margin) return;
    itemMargin = margin;
    auto* el = uiManager->findElementByName(uiElementId);
    if (el && el->rect.h > 0) {
        int slotH = itemHeight + itemMargin;
        maxVisibleItems = (slotH > 0) ? (el->rect.h / slotH + 2) : 2;
    }
    updateVisibleItems();
}

void BannerListWidget::setBackgroundImage(const std::string& imageName) {
    if (backgroundImageName == imageName) return;
    backgroundImageName = imageName;
    updateVisibleItems();
}

void BannerListWidget::setBackgroundColor(SDL_Color color) {
    if (backgroundColor.r == color.r && backgroundColor.g == color.g &&
        backgroundColor.b == color.b && backgroundColor.a == color.a) return;
    backgroundColor = color;
    updateVisibleItems();
}

void BannerListWidget::setFontSize(int size) {
    if (fontSize == size) return;
    fontSize = size;
    updateVisibleItems();
}

void BannerListWidget::setTextColor(SDL_Color color) {
    if (textColor.r == color.r && textColor.g == color.g &&
        textColor.b == color.b && textColor.a == color.a) return;
    textColor = color;
    updateVisibleItems();
}

void BannerListWidget::setTextAlign(BannerTextAlignH h, BannerTextAlignV v) {
    if (textAlignH == h && textAlignV == v) return;
    textAlignH = h;
    textAlignV = v;
    updateVisibleItems();
}

void BannerListWidget::setScrollOffset(int offset) {
    scrollOffset = offset;
    auto* el = uiManager->findElementByName(uiElementId);
    if (!el) return;
    int widgetH = el->rect.h;
    int slotH = itemHeight + itemMargin;
    int totalH = (items.empty()) ? 0 : (static_cast<int>(items.size()) * slotH - itemMargin);
    int maxScroll = totalH - widgetH;
    if (maxScroll < 0) maxScroll = 0;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;
    if (scrollOffset < 0) scrollOffset = 0;
    updateItemPositions();
    updateVisibleItems();
}

void BannerListWidget::scrollToTop() {
    scrollOffset = 0;
    updateItemPositions();
    updateVisibleItems();
}

void BannerListWidget::scrollToBottom() {
    auto* el = uiManager->findElementByName(uiElementId);
    if (!el) return;
    int widgetH = el->rect.h;
    int slotH = itemHeight + itemMargin;
    int totalH = (items.empty()) ? 0 : (static_cast<int>(items.size()) * slotH - itemMargin);
    int maxScroll = totalH - widgetH;
    if (maxScroll < 0) maxScroll = 0;
    scrollOffset = maxScroll;
    updateItemPositions();
    updateVisibleItems();
}

SDL_Texture* BannerListWidget::createBackgroundTexture(int w, int h, const std::string& imageName,
                                                      std::string& outTextureId) {
    outTextureId = "";
    if (w <= 0 || h <= 0) return nullptr;

    ImageRenderer imgRenderer(renderer, w, h);

    std::string imgToUse = !imageName.empty() ? imageName : backgroundImageName;
    if (!imgToUse.empty()) {
        SDL_Texture* baseTex = resourceManager->getTexture(imgToUse);
        if (baseTex) {
            int tw, th;
            SDL_QueryTexture(baseTex, nullptr, nullptr, &tw, &th);
            imgRenderer.drawImageScaled(baseTex, 0, 0, w, h, 0, 0, tw, th);
        } else {
            imgRenderer.drawColor(backgroundColor);
        }
    } else {
        imgRenderer.drawColor(backgroundColor);
    }

    SDL_Texture* finalTex = imgRenderer.getTexture();
    if (!finalTex) return nullptr;

    int tw, th;
    SDL_QueryTexture(finalTex, nullptr, nullptr, &tw, &th);
    SDL_Texture* copiedTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                               SDL_TEXTUREACCESS_TARGET, tw, th);
    if (copiedTex) {
        SDL_SetTextureBlendMode(copiedTex, SDL_BLENDMODE_BLEND);
        SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer, copiedTex);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, finalTex, nullptr, nullptr);
        SDL_SetRenderTarget(renderer, oldTarget);
        outTextureId = resourceManager->registerTexture(copiedTex);
        return copiedTex;
    }
    return nullptr;
}

void BannerListWidget::computeTextPosition(int itemW, int itemH, int textW, int textH,
                                          int& outX, int& outY) {
    switch (textAlignH) {
        case BannerTextAlignH::Left:
            outX = TEXT_PADDING;
            break;
        case BannerTextAlignH::Center:
            outX = (itemW - textW) / 2;
            if (outX < TEXT_PADDING) outX = TEXT_PADDING;
            break;
        case BannerTextAlignH::Right:
            outX = itemW - textW - TEXT_PADDING;
            if (outX < TEXT_PADDING) outX = TEXT_PADDING;
            break;
    }
    switch (textAlignV) {
        case BannerTextAlignV::Top:
            outY = TEXT_PADDING;
            break;
        case BannerTextAlignV::Center:
            outY = (itemH - textH) / 2;
            if (outY < TEXT_PADDING) outY = TEXT_PADDING;
            break;
        case BannerTextAlignV::Bottom:
            outY = itemH - textH - TEXT_PADDING;
            if (outY < TEXT_PADDING) outY = TEXT_PADDING;
            break;
    }
}

void BannerListWidget::handleEvent(const SDL_Event& event) {
    auto* el = uiManager->findElementByName(uiElementId);
    if (!el) return;

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int x = event.button.x;
        int y = event.button.y;
        wasDraggableOnMouseDown = isPointInside(x, y);
        if (!wasDraggableOnMouseDown) return;
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
    else if (event.type == MOUSE_CLICK_EVENT && event.user.code == SDL_BUTTON_LEFT) {
        if (!onItemClick) return;

        int* coords = static_cast<int*>(event.user.data1);
        if (!coords) return;
        int x = coords[0];
        int y = coords[1];

        if (!isPointInside(x, y)) return;

        for (auto& vi : visibleItems) {
            if (vi.bgElementId.empty()) continue;
            auto* bgEl = uiManager->findElementByName(vi.bgElementId);
            if (!bgEl || !bgEl->visible) continue;

            int bx, by;
            uiManager->getWorldPosition(vi.bgElementId, bx, by);
            int bw = bgEl->rect.w;
            int bh = bgEl->rect.h;

            if (x >= bx && x < bx + bw && y >= by && y < by + bh) {
                onItemClick(vi.itemIndex);
                break;
            }
        }
    }
}

void BannerListWidget::update(float deltaTime) {}

void BannerListWidget::updateVisibleItems() {
    if (items.empty()) {
        clearVisibleItems();
        return;
    }

    auto* el = uiManager->findElementByName(uiElementId);
    if (!el) return;

    int viewportH = el->rect.h;
    int itemW = el->rect.w;
    if (viewportH <= 0 || itemW <= 0) return;

    int slotH = itemHeight + itemMargin;
    int startIndex = (slotH > 0) ? (scrollOffset / slotH) : 0;
    if (startIndex < 0) startIndex = 0;
    if (startIndex >= static_cast<int>(items.size())) {
        clearVisibleItems();
        return;
    }

    int endIndex = startIndex + ((slotH > 0) ? (viewportH / slotH) : 0) + 2;
    endIndex = std::min(endIndex, static_cast<int>(items.size()) - 1);
    if (endIndex < startIndex) {
        clearVisibleItems();
        return;
    }

    std::set<int> currentVisible;
    for (int i = startIndex; i <= endIndex; i++) currentVisible.insert(i);

    for (auto it = visibleItems.begin(); it != visibleItems.end();) {
        if (currentVisible.find(it->itemIndex) == currentVisible.end()) {
            if (!it->bgElementId.empty()) {
                uiManager->removeUI(it->bgElementId);
                if (!it->bgTextureId.empty()) {
                    resourceManager->unregisterTexture(it->bgTextureId);
                }
            }
            if (!it->textElementId.empty()) {
                uiManager->removeUI(it->textElementId);
                if (!it->textTextureId.empty()) {
                    resourceManager->unregisterTexture(it->textTextureId);
                }
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

void BannerListWidget::createItemElement(int index) {
    if (index < 0 || index >= static_cast<int>(items.size())) return;

    const BannerItem& item = items[index];
    const std::string& title = item.title;
    auto* parentEl = uiManager->findElementByName(uiElementId);
    if (!parentEl) return;

    int itemW = parentEl->rect.w;
    if (itemW <= 0) return;

    VisibleItem vi;
    vi.itemIndex = index;

    int slotH = itemHeight + itemMargin;
    int baseY = index * slotH - scrollOffset;

    SDL_Texture* bgTex = createBackgroundTexture(itemW, itemHeight, item.imageName, vi.bgTextureId);
    if (bgTex) {
        UIElement bgEl;
        bgEl.name = "";
        bgEl.texture = bgTex;
        bgEl.textureId = vi.bgTextureId;
        bgEl.rect = SDL_Rect{0, baseY, itemW, itemHeight};
        bgEl.visible = true;
        bgEl.clickable = true;

        vi.bgElementId = uiManager->addUIAndGetId(bgEl);
        uiManager->setParent(vi.bgElementId, uiElementId);
    }

    int textMaxW = itemW - 2 * TEXT_PADDING;
    int textMaxH = itemHeight - 2 * TEXT_PADDING;
    if (textMaxW < 10) textMaxW = itemW;
    if (textMaxH < 10) textMaxH = itemHeight;

    SDL_Texture* textTex = nullptr;
    if (!title.empty() && textRenderer) {
        textTex = textRenderer->renderText(renderer, title, fontSize, textColor, textMaxW, textMaxH);
    }
    if (textTex) {
        vi.textTextureId = resourceManager->registerTexture(textTex);

        int textW, textH;
        SDL_QueryTexture(textTex, nullptr, nullptr, &textW, &textH);

        int tx, ty;
        computeTextPosition(itemW, itemHeight, textW, textH, tx, ty);

        UIElement textEl;
        textEl.name = "";
        textEl.texture = textTex;
        textEl.textureId = vi.textTextureId;
        textEl.rect = SDL_Rect{tx, baseY + ty, textW, textH};
        textEl.visible = true;
        textEl.clickable = false;

        vi.textElementId = uiManager->addUIAndGetId(textEl);
        uiManager->setParent(vi.textElementId, uiElementId);
    }

    visibleItems.push_back(std::move(vi));
}

void BannerListWidget::updateItemPositions() {
    for (auto& vi : visibleItems) {
        if (vi.itemIndex < 0 || vi.itemIndex >= static_cast<int>(items.size()))
            continue;

        int slotH = itemHeight + itemMargin;
        int baseY = vi.itemIndex * slotH - scrollOffset;

        if (!vi.bgElementId.empty()) {
            auto* el = uiManager->findElementByName(vi.bgElementId);
            if (el) {
                el->rect.y = baseY;
            }
        }

        if (!vi.textElementId.empty()) {
            auto* parentEl = uiManager->findElementByName(uiElementId);
            if (!parentEl) continue;
            int itemW = parentEl->rect.w;

            auto* textEl = uiManager->findElementByName(vi.textElementId);
            if (textEl) {
                int textW = textEl->rect.w;
                int textH = textEl->rect.h;
                int tx, ty;
                computeTextPosition(itemW, itemHeight, textW, textH, tx, ty);
                textEl->rect.x = tx;
                textEl->rect.y = baseY + ty;
            }
        }
    }
}

void BannerListWidget::clearVisibleItems() {
    for (auto& vi : visibleItems) {
        if (!vi.bgElementId.empty()) {
            uiManager->removeUI(vi.bgElementId);
            if (!vi.bgTextureId.empty()) {
                resourceManager->unregisterTexture(vi.bgTextureId);
            }
        }
        if (!vi.textElementId.empty()) {
            uiManager->removeUI(vi.textElementId);
            if (!vi.textTextureId.empty()) {
                resourceManager->unregisterTexture(vi.textTextureId);
            }
        }
    }
    visibleItems.clear();
}
