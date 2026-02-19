#include "TextRenderer.h"


bool TextRenderer::init(const std::string& fontPath) {
    if (TTF_Init() < 0) {
        SDL_Log("TTF_Init Error: %s", TTF_GetError());
        return false;
    }

    fontFile = fontPath;
    return true;
}

SDL_Texture* TextRenderer::renderText(SDL_Renderer* renderer, const std::string& text,
                                      int fontSize, SDL_Color color,
                                      int wrapWidth, int maxHeight) {
    TTF_Font* font = TTF_OpenFont(fontFile.c_str(), fontSize);
    if (!font) {
        SDL_Log("TTF_OpenFont Error: %s", TTF_GetError());
        return nullptr;
    }

    SDL_Surface* fullSurface = nullptr;
    if (wrapWidth > 0) {
        // UTF-8 텍스트 렌더링 (한글 지원)
        fullSurface = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), color, wrapWidth);
    } else {
        // UTF-8 텍스트 렌더링 (한글 지원)
        fullSurface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    }

    if (!fullSurface) {
        SDL_Log("TTF_RenderText Error: %s", TTF_GetError());
        TTF_CloseFont(font);
        return nullptr;
    }

    // Height 클리핑 처리
    SDL_Surface* clippedSurface = fullSurface;
    if (maxHeight > 0 && fullSurface->h > maxHeight) {
        clippedSurface = SDL_CreateRGBSurfaceWithFormat(0, fullSurface->w, maxHeight, 32, fullSurface->format->format);
        SDL_Rect srcRect = {0, 0, fullSurface->w, maxHeight};
        SDL_BlitSurface(fullSurface, &srcRect, clippedSurface, nullptr);
        SDL_FreeSurface(fullSurface);
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, clippedSurface);
    if (!texture) {
        SDL_Log("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
    }

    if (clippedSurface != fullSurface) {
        SDL_FreeSurface(clippedSurface);
    }

    TTF_CloseFont(font);
    return texture;
}

bool TextRenderer::getTextSize(const std::string& text, int fontSize, int* outW, int* outH) {
    if (!outW || !outH || text.empty()) return false;
    TTF_Font* font = TTF_OpenFont(fontFile.c_str(), fontSize);
    if (!font) return false;
    int ret = TTF_SizeUTF8(font, text.c_str(), outW, outH);
    TTF_CloseFont(font);
    return (ret == 0);
}

void TextRenderer::quit() {
    TTF_Quit();
}