#pragma once
#include "../UIWidget.h"
#include <vector>
#include <string>
#include "../../utils/sdl_includes.h"

class TilemapWidget : public UIWidget {
private:
    std::vector<std::vector<int>> tileData;  // 타일 ID 그리드 [y][x]
    SDL_Texture* tilesetTexture;             // 타일셋 텍스처
    std::string tilesetTextureName;          // 타일셋 텍스처 이름
    
    int tileSize;                             // 타일 크기 (픽셀)
    int mapWidth;                             // 맵 너비 (타일 개수)
    int mapHeight;                            // 맵 높이 (타일 개수)
    int tilesetColumns;                       // 타일셋의 열 개수
    
    int offsetX;                              // 렌더링 오프셋 X
    int offsetY;                              // 렌더링 오프셋 Y
    
    SDL_Renderer* renderer;                   // 렌더러 (직접 렌더링용)
    std::string uiElementId;                  // UIElement ID
    
    // 타일 ID로 소스 rect 계산
    SDL_Rect getTileSourceRect(int tileId) const;
    
public:
    std::string getUIElementIdentifier() const override { return uiElementId; }
    TilemapWidget(UiManager* uiMgr, ResourceManager* resMgr,
                  SDL_Renderer* sdlRenderer,
                  const std::string& tilesetName, int tileSize, int tilesetColumns,
                  int mapWidth, int mapHeight,
                  const SDL_Rect& rect = SDL_Rect{0, 0, 0, 0},
                  float scale = 1.0f, float rotation = 0.0f, float alpha = 1.0f,
                  bool visible = true, bool clickable = false);
    
    virtual ~TilemapWidget();
    
    // 타일 데이터 설정
    void setTileData(const std::vector<std::vector<int>>& data);
    void setTile(int x, int y, int tileId);
    int getTile(int x, int y) const;
    
    // 오프셋 설정 (카메라 위치)
    void setOffset(int x, int y);
    void getOffset(int& x, int& y) const;
    
    // 맵 크기
    int getMapWidth() const { return mapWidth; }
    int getMapHeight() const { return mapHeight; }
    int getTileSize() const { return tileSize; }
    
    // 렌더링 (직접 렌더링)
    void render(SDL_Renderer* renderer) override;
};




