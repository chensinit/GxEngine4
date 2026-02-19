#include "TilemapWidget.h"
#include "../../ui/uiManager.h"
#include "../../resource/resourceManager.h"
#include "../../utils/logger.h"

TilemapWidget::TilemapWidget(UiManager* uiMgr, ResourceManager* resMgr,
                             SDL_Renderer* sdlRenderer,
                             const std::string& tilesetName, int tileSize, int tilesetColumns,
                             int mapWidth, int mapHeight, const SDL_Rect& rect,
                             float scale, float rotation, float alpha, bool visible, bool clickable)
    : UIWidget(uiMgr, resMgr),
      tilesetTexture(nullptr),
      tilesetTextureName(tilesetName),
      tileSize(tileSize),
      mapWidth(mapWidth),
      mapHeight(mapHeight),
      tilesetColumns(tilesetColumns),
      offsetX(0),
      offsetY(0),
      renderer(sdlRenderer),
      uiElementId("") {
    
    // 타일셋 텍스처 로드
    tilesetTexture = resourceManager->getTexture(tilesetName);
    if (!tilesetTexture) {
        Log::error("[TilemapWidget] Tileset texture not found: ", tilesetName);
    }
    
    // 타일 데이터 초기화 (빈 맵)
    tileData.resize(mapHeight);
    for (int y = 0; y < mapHeight; y++) {
        tileData[y].resize(mapWidth, 0);  // 기본값 0 (빈 타일)
    }
    
    // UIElement 생성 (렌더링 영역 및 클릭 감지용)
    UIElement element;
    element.name = "";  // 빈 이름으로 자동 ID 생성
    element.texture = nullptr;  // TilemapWidget은 직접 렌더링
    element.rect = rect;
    element.visible = visible;
    element.clickable = clickable;
    element.scale = scale;
    element.rotation = rotation;
    element.alpha = alpha;
    uiElementId = uiManager->addUIAndGetId(element);
}

TilemapWidget::~TilemapWidget() {
    // 텍스처는 ResourceManager에서 관리하므로 여기서 해제하지 않음
}

void TilemapWidget::setTileData(const std::vector<std::vector<int>>& data) {
    if (data.size() != static_cast<size_t>(mapHeight)) {
        Log::error("[TilemapWidget] Invalid tile data height: ", data.size(), 
                  " expected: ", mapHeight);
        return;
    }
    
    for (int y = 0; y < mapHeight; y++) {
        if (data[y].size() != static_cast<size_t>(mapWidth)) {
            Log::error("[TilemapWidget] Invalid tile data width at row ", y, 
                      ": ", data[y].size(), " expected: ", mapWidth);
            return;
        }
    }
    
    tileData = data;
}

void TilemapWidget::setTile(int x, int y, int tileId) {
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
        Log::error("[TilemapWidget] Tile position out of bounds: (", x, ", ", y, ")");
        return;
    }
    
    tileData[y][x] = tileId;
}

int TilemapWidget::getTile(int x, int y) const {
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
        return -1;
    }
    
    return tileData[y][x];
}

void TilemapWidget::setOffset(int x, int y) {
    offsetX = x;
    offsetY = y;
}

void TilemapWidget::getOffset(int& x, int& y) const {
    x = offsetX;
    y = offsetY;
}

SDL_Rect TilemapWidget::getTileSourceRect(int tileId) const {
    if (tileId < 0 || tilesetTexture == nullptr) {
        return {0, 0, 0, 0};  // 빈 rect
    }
    
    // 타일셋 텍스처 크기 가져오기
    int textureWidth, textureHeight;
    if (SDL_QueryTexture(tilesetTexture, nullptr, nullptr, &textureWidth, &textureHeight) != 0) {
        return {0, 0, 0, 0};  // 텍스처 정보를 가져올 수 없음
    }
    
    // 타일셋의 행 개수 계산
    int tilesetRows = textureHeight / tileSize;
    int maxTileId = tilesetColumns * tilesetRows - 1;
    
    // 타일 ID가 범위를 벗어나면 빈 rect 반환
    if (tileId > maxTileId) {
        return {0, 0, 0, 0};
    }
    
    // 타일셋에서 타일 위치 계산
    int tileX = tileId % tilesetColumns;
    int tileY = tileId / tilesetColumns;
    
    // 타일이 텍스처 범위를 벗어나는지 확인
    if (tileY >= tilesetRows) {
        return {0, 0, 0, 0};
    }
    
    return {
        tileX * tileSize,
        tileY * tileSize,
        tileSize,
        tileSize
    };
}

void TilemapWidget::render(SDL_Renderer* renderer) {
    if (!tilesetTexture) {
        Log::error("[TilemapWidget] render() called but tilesetTexture is null!");
        return;  // 타일셋이 없으면 렌더링하지 않음
    }
    
    if (tileData.empty() || tileData[0].empty()) {
        Log::error("[TilemapWidget] render() called but tileData is empty!");
        return;
    }
    
    // 위젯의 위치와 크기 가져오기
    int widgetX = 0, widgetY = 0;
    int widgetWidth = 800, widgetHeight = 600;  // 기본값
    
    if (uiManager) {
        auto* element = uiManager->findElementByName(uiElementId);
        if (element) {
            // 월드 위치 계산 (부모 위치 누적)
            uiManager->getWorldPosition(uiElementId, widgetX, widgetY);
            widgetWidth = static_cast<int>(element->rect.w * element->scale);
            widgetHeight = static_cast<int>(element->rect.h * element->scale);
        }
    }
    
    // 위젯 영역에 보이는 타일 범위 계산
    // offsetX/Y가 음수면 맵을 오른쪽/아래로 이동 (카메라가 왼쪽/위로 이동)
    int startX = (-offsetX) / tileSize;
    int startY = (-offsetY) / tileSize;
    int endX = startX + (widgetWidth / tileSize) + 2;  // 여유분 추가
    int endY = startY + (widgetHeight / tileSize) + 2;
    
    // 범위 제한
    if (startX < 0) startX = 0;
    if (startY < 0) startY = 0;
    if (endX > mapWidth) endX = mapWidth;
    if (endY > mapHeight) endY = mapHeight;
    
    // 렌더링된 타일 개수 카운트
    int renderedCount = 0;
    
    // 타일 렌더링
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            if (y >= static_cast<int>(tileData.size()) || x >= static_cast<int>(tileData[y].size())) {
                continue;
            }
            
            int tileId = tileData[y][x];
            if (tileId <= 0) continue;  // 0 이하는 빈 타일로 간주
            
            // 소스 rect 계산
            SDL_Rect srcRect = getTileSourceRect(tileId);
            if (srcRect.w == 0 || srcRect.h == 0) continue;
            
            // 목적지 rect 계산 (위젯 위치 + 월드 좌표 + 오프셋)
            SDL_Rect dstRect = {
                widgetX + x * tileSize + offsetX,
                widgetY + y * tileSize + offsetY,
                tileSize,
                tileSize
            };
            
            // 렌더링
            SDL_RenderCopy(renderer, tilesetTexture, &srcRect, &dstRect);
            renderedCount++;
        }
    }
}

