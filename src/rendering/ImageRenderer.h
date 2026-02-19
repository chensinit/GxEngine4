#pragma once
#include "../utils/sdl_includes.h"
#include <string>

class TextRenderer;

class ImageRenderer {
private:
    SDL_Surface* surface;      // 그리기 작업을 수행할 surface
    SDL_Renderer* renderer;    // 텍스처 생성용
    int width;
    int height;
    bool needsUpdate;          // 텍스처 업데이트 필요 여부
    SDL_Texture* cachedTexture; // 캐시된 텍스처
    
    // 내부 헬퍼 함수들
    void updateTexture();      // surface를 texture로 변환
    void lockSurface();        // surface 잠금
    void unlockSurface();      // surface 잠금 해제
    
public:
    ImageRenderer(SDL_Renderer* sdlRenderer, int w, int h);
    ~ImageRenderer();
    
    // 복사 생성자 및 대입 연산자 비활성화 (surface 관리 복잡성 때문)
    ImageRenderer(const ImageRenderer&) = delete;
    ImageRenderer& operator=(const ImageRenderer&) = delete;
    
    // 기본 그리기 함수
    void drawPixel(int x, int y, SDL_Color color);
    void drawColor(SDL_Color color);  // 전체 이미지를 하나의 색으로 채우기
    
    // 텍스트 그리기
    void drawText(const std::string& text, int x, int y, int fontSize, SDL_Color color, TextRenderer* textRenderer, int maxWidth = 0);
    
    // 도형 그리기
    void drawRect(int x, int y, int w, int h, SDL_Color color, bool filled = true);
    void drawCircle(int centerX, int centerY, int radius, SDL_Color color, bool filled = true);
    
    // 이미지 그리기
    void drawImage(SDL_Surface* srcSurface, int dstX, int dstY, int srcX = 0, int srcY = 0, int srcW = -1, int srcH = -1);
    void drawImage(SDL_Texture* srcTexture, int dstX, int dstY, int srcX = 0, int srcY = 0, int srcW = -1, int srcH = -1);
    // 이미지 스케일 그리기 (원본을 dstW x dstH로 확대/축소)
    void drawImageScaled(SDL_Texture* srcTexture, int dstX, int dstY, int dstW, int dstH, int srcX = 0, int srcY = 0, int srcW = -1, int srcH = -1);
    
    // 이미지 가져오기
    SDL_Texture* getTexture();  // 텍스처 반환 (화면 출력용)
    SDL_Surface* getSurface() const { return surface; }  // surface 직접 접근
    
    // 크기 정보
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
    // 초기화 (전체 지우기)
    void clear(SDL_Color color = {0, 0, 0, 255});
};


