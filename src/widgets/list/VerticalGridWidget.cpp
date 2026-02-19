#include "VerticalGridWidget.h"
#include "../../ui/uiManager.h"
#include "../../scene.h"
#include "../../resource/resourceManager.h"
#include "../../utils/logger.h"
#include <algorithm>

VerticalGridWidget::VerticalGridWidget(UiManager* uiMgr, ResourceManager* resMgr,
                                       int cols, int cellW, int cellH,
                                       const SDL_Rect& rect,
                                       const std::string& cellBgImage,
                                       int cellMarg,
                                       float scale, float rotation, float alpha,
                                       bool visible, bool clickable)
    : UIWidget(uiMgr, resMgr),
      columns(std::max(1, cols)),
      cellWidth(std::max(1, cellW)),
      cellHeight(std::max(1, cellH)),
      cellMargin(std::max(0, cellMarg)),
      cellMarginH(0),
      scrollOffset(0),
      cellBackgroundImage(cellBgImage),
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
}

VerticalGridWidget::~VerticalGridWidget() {
    clearVisibleCells();
}

void VerticalGridWidget::addItem(const std::string& imageName) {
    items.push_back(imageName);
    updateVisibleCells();
}

void VerticalGridWidget::clearItems() {
    clearVisibleCells();
    items.clear();
    scrollOffset = 0;
}

void VerticalGridWidget::setColumns(int cols) {
    if (columns == std::max(1, cols)) return;
    columns = std::max(1, cols);
    updateVisibleCells();
}

void VerticalGridWidget::setCellSize(int w, int h) {
    if (cellWidth == w && cellHeight == h) return;
    cellWidth = std::max(1, w);
    cellHeight = std::max(1, h);
    updateVisibleCells();
}

void VerticalGridWidget::setScrollOffset(int offset) {
    auto* element = uiManager->findElementByName(uiElementId);
    if (!element) return;
    
    int widgetH = element->rect.h;
    int totalRows = (static_cast<int>(items.size()) + columns - 1) / columns;
    int contentH = totalRows * cellHeight;
    int maxScroll = contentH - widgetH;
    if (maxScroll < 0) maxScroll = 0;
    
    scrollOffset = std::max(0, std::min(offset, maxScroll));
    updateCellPositions();
    updateVisibleCells();
}

void VerticalGridWidget::scrollToTop() {
    scrollOffset = 0;
    updateCellPositions();
    updateVisibleCells();
}

void VerticalGridWidget::scrollToBottom() {
    auto* element = uiManager->findElementByName(uiElementId);
    if (!element) return;
    
    int widgetH = element->rect.h;
    int totalRows = (static_cast<int>(items.size()) + columns - 1) / columns;
    int contentH = totalRows * cellHeight;
    scrollOffset = std::max(0, contentH - widgetH);
    updateCellPositions();
    updateVisibleCells();
}

void VerticalGridWidget::handleEvent(const SDL_Event& event) {
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

void VerticalGridWidget::update(float deltaTime) {
    (void)deltaTime;
}

void VerticalGridWidget::updateVisibleCells() {
    auto* element = uiManager->findElementByName(uiElementId);
    if (!element) return;
    
    int widgetH = element->rect.h;
    int totalRows = (static_cast<int>(items.size()) + columns - 1) / columns;
    
    int startRow = scrollOffset / cellHeight;
    int endRow = (scrollOffset + widgetH) / cellHeight + 1;
    if (endRow > totalRows) endRow = totalRows;
    
    int startIndex = startRow * columns;
    int endIndex = endRow * columns;
    if (endIndex > static_cast<int>(items.size())) {
        endIndex = static_cast<int>(items.size());
    }
    
    for (auto it = visibleCells.begin(); it != visibleCells.end(); ) {
        if (it->itemIndex < startIndex || it->itemIndex >= endIndex) {
            int idxToRemove = it->itemIndex;
            removeCellElement(idxToRemove);
            it = visibleCells.erase(it);
        } else {
            ++it;
        }
    }
    
    for (int i = startIndex; i < endIndex; i++) {
        bool found = false;
        for (const auto& v : visibleCells) {
            if (v.itemIndex == i) { found = true; break; }
        }
        if (!found) {
            createCellElement(i);
        }
    }
    
    updateCellPositions();
}

void VerticalGridWidget::createCellElement(int index) {
    if (index < 0 || index >= static_cast<int>(items.size())) return;
    
    const std::string& imageName = items[index];
    SDL_Texture* tex = resourceManager->getTexture(imageName);
    if (!tex) return;
    
    int row = index / columns;
    int col = index % columns;
    int marginH = (cellMarginH > 0) ? cellMarginH : cellMargin;
    int marginV = cellMargin;
    int baseX = col * cellWidth + marginH;
    int baseY = row * cellHeight - scrollOffset + marginV;
    int frameW = cellWidth - 2 * marginH;
    int frameH = cellHeight - 2 * marginV;
    
    std::string frameId;
    std::string parentForImage = uiElementId;
    
    if (!cellBackgroundImage.empty()) {
        SDL_Texture* frameTex = resourceManager->getTexture(cellBackgroundImage);
        if (frameTex) {
            UIElement frameElement;
            frameElement.name = "";
            frameElement.texture = frameTex;
            frameElement.textureId = cellBackgroundImage;
            frameElement.rect = SDL_Rect{baseX, baseY, frameW, frameH};
            frameElement.visible = true;
            frameElement.clickable = false;
            frameId = uiManager->addUIAndGetId(frameElement);
            uiManager->setParent(frameId, uiElementId);
            parentForImage = frameId;
        }
    }
    
    int innerW = static_cast<int>(frameW * 0.80f);
    int innerH = static_cast<int>(frameH * 0.80f);
    int origW, origH;
    SDL_QueryTexture(tex, nullptr, nullptr, &origW, &origH);
    float imgScale = std::min(static_cast<float>(innerW) / origW,
                             static_cast<float>(innerH) / origH);
    int imgW = static_cast<int>(origW * imgScale);
    int imgH = static_cast<int>(origH * imgScale);
    int imgX = (frameW - imgW) / 2;
    int imgY = (frameH - imgH) / 2;
    
    UIElement imageElement;
    imageElement.name = "";
    imageElement.texture = tex;
    imageElement.textureId = imageName;
    imageElement.rect = SDL_Rect{imgX, imgY, imgW, imgH};
    imageElement.visible = true;
    imageElement.clickable = false;
    
    std::string imageId = uiManager->addUIAndGetId(imageElement);
    uiManager->setParent(imageId, parentForImage);
    
    VisibleCell vc;
    vc.itemIndex = index;
    vc.frameElementId = frameId;
    vc.imageElementId = imageId;
    visibleCells.push_back(vc);
}

void VerticalGridWidget::removeCellElement(int index) {
    for (auto it = visibleCells.begin(); it != visibleCells.end(); ++it) {
        if (it->itemIndex == index) {
            if (!it->imageElementId.empty()) {
                uiManager->removeUI(it->imageElementId);
            }
            if (!it->frameElementId.empty()) {
                uiManager->removeUI(it->frameElementId);
            }
            break;
        }
    }
}

void VerticalGridWidget::updateCellPositions() {
    int marginH = (cellMarginH > 0) ? cellMarginH : cellMargin;
    int marginV = cellMargin;
    int frameW = cellWidth - 2 * marginH;
    int frameH = cellHeight - 2 * marginV;
    
    for (auto& vc : visibleCells) {
        int index = vc.itemIndex;
        int row = index / columns;
        int col = index % columns;
        int baseX = col * cellWidth + marginH;
        int baseY = row * cellHeight - scrollOffset + marginV;
        
        if (!vc.frameElementId.empty()) {
            auto* frameElement = uiManager->findElementByName(vc.frameElementId);
            if (frameElement) {
                frameElement->rect.x = baseX;
                frameElement->rect.y = baseY;
                frameElement->rect.w = frameW;
                frameElement->rect.h = frameH;
            }
        }
        
        if (!vc.imageElementId.empty()) {
            auto* imgElement = uiManager->findElementByName(vc.imageElementId);
            if (imgElement && vc.frameElementId.empty()) {
                int imgW = imgElement->rect.w;
                int imgH = imgElement->rect.h;
                imgElement->rect.x = baseX + (frameW - imgW) / 2;
                imgElement->rect.y = baseY + (frameH - imgH) / 2;
            }
        }
    }
}

void VerticalGridWidget::clearVisibleCells() {
    for (auto& vc : visibleCells) {
        if (!vc.imageElementId.empty()) {
            uiManager->removeUI(vc.imageElementId);
        }
        if (!vc.frameElementId.empty()) {
            uiManager->removeUI(vc.frameElementId);
        }
    }
    visibleCells.clear();
}

void VerticalGridWidget::setCellMargin(int margin) {
    if (cellMargin == std::max(0, margin)) return;
    cellMargin = std::max(0, margin);
    updateVisibleCells();
}

void VerticalGridWidget::setCellMarginH(int marginH) {
    if (cellMarginH == std::max(0, marginH)) return;
    cellMarginH = std::max(0, marginH);
    updateVisibleCells();
}

void VerticalGridWidget::setCellBackgroundImage(const std::string& imageName) {
    if (cellBackgroundImage == imageName) return;
    cellBackgroundImage = imageName;
    updateVisibleCells();
}
