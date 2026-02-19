#pragma once
#include "../utils/sdl_includes.h"
#include <string>

class TextRenderer {
public:
    bool init(const std::string& fontPath);
    SDL_Texture* renderText(SDL_Renderer* renderer, const std::string& text,
                            int fontSize, SDL_Color color, int wrapWidth = 0, int maxHeight = -1);
    // 텍스트 크기 조회 (렌더링 없이, 단일 라인 기준)
    bool getTextSize(const std::string& text, int fontSize, int* outW, int* outH);

    void quit();

private:
    std::string fontFile;
};