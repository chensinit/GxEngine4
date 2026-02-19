#include "SectionGridWidget.h"
#include "../../ui/uiManager.h"
#include "../../resource/resourceManager.h"
#include "../../rendering/TextRenderer.h"
#include "../../utils/logger.h"
#include <algorithm>

SectionGridWidget::SectionGridWidget(UiManager* uiMgr,
                                     ResourceManager* resMgr,
                                     SDL_Renderer* sdlRenderer,
                                     TextRenderer* txtRenderer,
                                     int cols,
                                     int cellW,
                                     int cellH,
                                     const SDL_Rect& rect,
                                     int cellMarg,
                                     int headerMargV,
                                     float scale,
                                     float rotation,
                                     float alpha,
                                     bool visible,
                                     bool clickable)
    : UIWidget(uiMgr, resMgr),
      columns(std::max(1, cols)),
      cellWidth(std::max(1, cellW)),
      cellHeight(std::max(1, cellH)),
      cellMargin(std::max(0, cellMarg)),
      headerMarginV(std::max(0, headerMargV)),
      renderer(sdlRenderer),
      textRenderer(txtRenderer) {
    UIElement root;
    root.name = "";
    root.texture = nullptr;
    root.rect = rect;
    root.visible = visible;
    root.clickable = clickable;
    root.scale = scale;
    root.rotation = rotation;
    root.alpha = alpha;
    uiElementId = uiManager->addUIAndGetId(root);
}

SectionGridWidget::~SectionGridWidget() {
    clearElements();
}

void SectionGridWidget::clearElements() {
    for (auto& kv : createdElements) {
        CreatedElement& e = kv.second;
        if (!e.imageElementId.empty()) {
            uiManager->removeUI(e.imageElementId);
        }
        if (!e.bgElementId.empty()) {
            uiManager->removeUI(e.bgElementId);
        }
        if (!e.overlayElementId.empty()) {
            uiManager->removeUI(e.overlayElementId);
        }
        if (!e.textElementId.empty()) {
            uiManager->removeUI(e.textElementId);
        }
    }
    createdElements.clear();
    if (!contentElementId.empty()) {
        uiManager->removeUI(contentElementId);
        contentElementId.clear();
    }
}

void SectionGridWidget::clear() {
    clearElements();
    items.clear();
    layoutData.clear();
    scrollOffset = 0;
}

void SectionGridWidget::addOverlay(const std::string& overlayId,
                                  const std::string& imageName,
                                  int height) {
    OverlayDef def;
    def.imageName = imageName;
    def.height = std::max(1, height);
    overlays[overlayId] = def;
}

void SectionGridWidget::addHeader(const std::string& imageName) {
    Item item;
    item.kind = ItemKind::Header;
    item.headerImage = imageName;
    items.push_back(item);
}

void SectionGridWidget::addCard(const std::string& imageName,
                               const std::string& overlayId) {
    Item item;
    item.kind = ItemKind::Card;
    item.cardImage = imageName;
    item.overlayId = overlayId;
    items.push_back(item);
}

void SectionGridWidget::layout() {
    auto* rootElement = uiManager->findElementByName(uiElementId);
    if (!rootElement) return;

    int maxWidth = rootElement->rect.w;
    if (maxWidth <= 0) {
        maxWidth = columns * cellWidth;
    }

    for (auto& kv : createdElements) {
        CreatedElement& e = kv.second;
        if (!e.imageElementId.empty()) uiManager->removeUI(e.imageElementId);
        if (!e.bgElementId.empty()) uiManager->removeUI(e.bgElementId);
        if (!e.overlayElementId.empty()) uiManager->removeUI(e.overlayElementId);
        if (!e.textElementId.empty()) uiManager->removeUI(e.textElementId);
    }
    createdElements.clear();

    computeLayout(maxWidth);
    ensureContentContainer(maxWidth);
    updateVisibleItems();

    auto* contentEl = uiManager->findElementByName(contentElementId);
    if (contentEl) {
        contentEl->rect.h = totalContentHeight;
        contentEl->rect.y = -scrollOffset;
    }
}

void SectionGridWidget::setColumns(int cols) {
    cols = std::max(1, cols);
    if (columns == cols) return;
    columns = cols;
    layout();
}

void SectionGridWidget::setCellSize(int w, int h) {
    w = std::max(1, w);
    h = std::max(1, h);
    if (cellWidth == w && cellHeight == h) return;
    cellWidth = w;
    cellHeight = h;
    layout();
}

void SectionGridWidget::setCellMargin(int margin) {
    margin = std::max(0, margin);
    if (cellMargin == margin) return;
    cellMargin = margin;
    layout();
}

void SectionGridWidget::setHeaderMarginV(int marginV) {
    marginV = std::max(0, marginV);
    if (headerMarginV == marginV) return;
    headerMarginV = marginV;
    layout();
}

void SectionGridWidget::setHeaderImageScale(float scale) {
    scale = std::max(0.01f, std::min(1.0f, scale));
    if (headerImageScale == scale) return;
    headerImageScale = scale;
    layout();
}

void SectionGridWidget::setScrollOffset(int offset) {
    auto* rootElement = uiManager->findElementByName(uiElementId);
    if (!rootElement) return;

    int widgetH = rootElement->rect.h;
    int maxScroll = totalContentHeight - widgetH;
    if (maxScroll < 0) maxScroll = 0;

    int newOffset = std::max(0, std::min(offset, maxScroll));
    if (newOffset == scrollOffset) return;

    scrollOffset = newOffset;

    if (!contentElementId.empty()) {
        auto* contentEl = uiManager->findElementByName(contentElementId);
        if (contentEl) {
            contentEl->rect.y = -scrollOffset;
        }
        updateVisibleItems();
    }
}

void SectionGridWidget::update(float deltaTime) {
    (void)deltaTime;
}

void SectionGridWidget::computeLayout(int maxWidth) {
    layoutData.clear();
    layoutData.reserve(items.size());

    int originX = 0;
    int originY = 0;
    int currentY = originY;
    int currentCol = 0;
    int rowStartY = currentY;

    for (size_t i = 0; i < items.size(); ++i) {
        const Item& item = items[i];

        if (item.kind == ItemKind::Header) {
            ItemLayout ld;
            ld.y = currentY;

            if (currentCol != 0) {
                currentCol = 0;
                currentY = rowStartY + cellHeight + headerMarginV;
                rowStartY = currentY;
                ld.y = currentY;
            } else if (currentY != originY) {
                currentY += headerMarginV;
                rowStartY = currentY;
                ld.y = currentY;
            }

            int headerH = 0;
            if (!item.headerImage.empty()) {
                SDL_Texture* tex = resourceManager->getTexture(item.headerImage);
                if (tex) {
                    int texW = 0, texH = 0;
                    SDL_QueryTexture(tex, nullptr, nullptr, &texW, &texH);
                    if (texW > 0 && texH > 0) {
                        int headerW = static_cast<int>(maxWidth * headerImageScale);
                        headerH = static_cast<int>(
                            static_cast<float>(texH) * (static_cast<float>(headerW) / texW));
                        if (headerH <= 0) headerH = texH;
                        ld.headerW = headerW;
                        ld.headerH = headerH;
                        ld.headerX = (maxWidth - headerW) / 2;
                    }
                }
            }
            if (headerH <= 0) {
                headerH = cellHeight / 2;
            }

            ld.headerImage = item.headerImage;
            ld.height = headerH;
            layoutData.push_back(ld);

            currentY += headerH + headerMarginV;
            rowStartY = currentY;
            currentCol = 0;
        } else {
            int margin = cellMargin;
            int frameW = cellWidth - 2 * margin;
            int frameH = cellHeight - 2 * margin;

            if (currentCol == 0) {
                rowStartY = currentY;
            }

            int baseX = originX + currentCol * cellWidth + margin;
            int baseY = rowStartY + margin;

            ItemLayout ld;
            ld.y = rowStartY;
            ld.height = cellHeight;
            ld.baseX = baseX;
            ld.baseY = baseY;
            ld.frameW = frameW;
            ld.frameH = frameH;
            ld.col = currentCol;
            layoutData.push_back(ld);

            ++currentCol;
            if (currentCol >= columns) {
                currentCol = 0;
                currentY = rowStartY + cellHeight;
            }
        }
    }

    totalContentHeight = currentY;
}

void SectionGridWidget::ensureContentContainer(int maxWidth) {
    if (!contentElementId.empty()) return;

    UIElement content;
    content.name = "";
    content.texture = nullptr;
    content.rect = SDL_Rect{0, 0, maxWidth, totalContentHeight};
    content.visible = true;
    content.clickable = false;
    contentElementId = uiManager->addUIAndGetId(content);
    uiManager->setParent(contentElementId, uiElementId);
}

void SectionGridWidget::updateVisibleItems() {
    auto* rootElement = uiManager->findElementByName(uiElementId);
    if (!rootElement || contentElementId.empty() || layoutData.empty()) return;

    int widgetH = rootElement->rect.h;
    int maxWidth = rootElement->rect.w;
    if (maxWidth <= 0) maxWidth = columns * cellWidth;

    int visibleTop = scrollOffset - cellHeight;
    int visibleBottom = scrollOffset + widgetH + cellHeight;

    for (size_t i = 0; i < items.size() && i < layoutData.size(); ++i) {
        const Item& item = items[i];
        const ItemLayout& ld = layoutData[i];

        if (ld.height <= 0) {
            auto it = createdElements.find(i);
            if (it != createdElements.end()) {
                CreatedElement& e = it->second;
                if (!e.imageElementId.empty()) uiManager->removeUI(e.imageElementId);
                if (!e.bgElementId.empty()) uiManager->removeUI(e.bgElementId);
                if (!e.overlayElementId.empty()) uiManager->removeUI(e.overlayElementId);
                if (!e.textElementId.empty()) uiManager->removeUI(e.textElementId);
                createdElements.erase(it);
            }
            continue;
        }

        int itemBottom = ld.y + ld.height;
        bool inView = (itemBottom > visibleTop && ld.y < visibleBottom);

        if (inView) {
            if (createdElements.find(i) != createdElements.end()) continue;

            CreatedElement ce;
            ce.kind = item.kind;

            if (item.kind == ItemKind::Header) {
                if (!item.headerImage.empty()) {
                    SDL_Texture* tex = resourceManager->getTexture(item.headerImage);
                    if (tex) {
                        UIElement imgEl;
                        imgEl.name = "";
                        imgEl.texture = tex;
                        imgEl.textureId = item.headerImage;
                        imgEl.rect = SDL_Rect{ld.headerX, ld.y, ld.headerW, ld.headerH};
                        imgEl.visible = true;
                        imgEl.clickable = false;

                        std::string imgId = uiManager->addUIAndGetId(imgEl);
                        uiManager->setParent(imgId, contentElementId);
                        ce.imageElementId = imgId;
                    }
                }

                createdElements[i] = ce;
            } else {
                SDL_Texture* tex = resourceManager->getTexture(item.cardImage);
                if (!tex) continue;

                int origW = 0, origH = 0;
                SDL_QueryTexture(tex, nullptr, nullptr, &origW, &origH);
                float imgScale = std::min(
                    static_cast<float>(ld.frameW) / std::max(1, origW),
                    static_cast<float>(ld.frameH) / std::max(1, origH));
                int imgW = static_cast<int>(origW * imgScale);
                int imgH = static_cast<int>(origH * imgScale);
                int imgX = ld.baseX + (ld.frameW - imgW) / 2;
                int imgY = ld.baseY + (ld.frameH - imgH) / 2;

                UIElement imgEl;
                imgEl.name = "";
                imgEl.texture = tex;
                imgEl.textureId = item.cardImage;
                imgEl.rect = SDL_Rect{imgX, imgY, imgW, imgH};
                imgEl.visible = true;
                imgEl.clickable = false;

                std::string imgId = uiManager->addUIAndGetId(imgEl);
                uiManager->setParent(imgId, contentElementId);
                ce.imageElementId = imgId;

                if (!item.overlayId.empty()) {
                    auto it = overlays.find(item.overlayId);
                    if (it != overlays.end()) {
                        const OverlayDef& overlayDef = it->second;
                        SDL_Texture* overlayTex = resourceManager->getTexture(overlayDef.imageName);
                        if (overlayTex) {
                            int overlayH = overlayDef.height;
                            int overlayY = imgY + (imgH - overlayH) / 2;

                            UIElement overlayEl;
                            overlayEl.name = "";
                            overlayEl.texture = overlayTex;
                            overlayEl.textureId = overlayDef.imageName;
                            overlayEl.rect = SDL_Rect{imgX, overlayY, imgW, overlayH};
                            overlayEl.visible = true;
                            overlayEl.clickable = false;

                            std::string overlayId = uiManager->addUIAndGetId(overlayEl);
                            uiManager->setParent(overlayId, contentElementId);
                            ce.overlayElementId = overlayId;
                        }
                    }
                }

                createdElements[i] = ce;
            }
        } else {
            auto it = createdElements.find(i);
            if (it != createdElements.end()) {
                CreatedElement& e = it->second;
                if (!e.imageElementId.empty()) uiManager->removeUI(e.imageElementId);
                if (!e.bgElementId.empty()) uiManager->removeUI(e.bgElementId);
                if (!e.overlayElementId.empty()) uiManager->removeUI(e.overlayElementId);
                if (!e.textElementId.empty()) uiManager->removeUI(e.textElementId);
                createdElements.erase(it);
            }
        }
    }
}

void SectionGridWidget::handleEvent(const SDL_Event& event) {
    auto* element = uiManager->findElementByName(uiElementId);
    if (!element) return;

    int widgetX, widgetY;
    uiManager->getWorldPosition(uiElementId, widgetX, widgetY);
    int widgetW = element->rect.w;
    int widgetH = element->rect.h;

    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int mx = event.button.x;
        int my = event.button.y;
        if (mx >= widgetX && mx < widgetX + widgetW && my >= widgetY && my < widgetY + widgetH) {
            isDragging = true;
            lastMouseY = my;
            dragStartY = my;
        }
    } else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        isDragging = false;
    } else if (event.type == SDL_MOUSEMOTION && isDragging) {
        int my = event.motion.y;
        int deltaY = lastMouseY - my;
        lastMouseY = my;
        setScrollOffset(scrollOffset + deltaY);
    }
}
