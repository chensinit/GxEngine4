#include "UIWidget.h"
#include "../ui/uiManager.h"
#include "../resource/resourceManager.h"

UIWidget::UIWidget(UiManager* uiMgr, ResourceManager* resMgr)
    : uiManager(uiMgr), resourceManager(resMgr) {
}

bool UIWidget::isPointInside(int x, int y) const {
    std::string elementId = getUIElementIdentifier();
    if (elementId.empty() || !uiManager) return false;
    
    auto* element = uiManager->findElementByName(elementId);
    if (!element || !element->visible) return false;
    
    // 월드 좌표 계산
    int worldX, worldY;
    uiManager->getWorldPosition(elementId, worldX, worldY);
    
    // 월드 좌표 기준으로 rect 생성
    SDL_Rect worldRect = {
        worldX,
        worldY,
        static_cast<int>(element->rect.w * element->scale),
        static_cast<int>(element->rect.h * element->scale)
    };
    
    SDL_Point point{x, y};
    return SDL_PointInRect(&point, &worldRect) != SDL_FALSE;
}

