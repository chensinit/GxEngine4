#include "ImageRenderer.h"
#include "TextRenderer.h"
#include <cmath>
#include <algorithm>
#include <iostream>

ImageRenderer::ImageRenderer(SDL_Renderer* sdlRenderer, int w, int h)
    : renderer(sdlRenderer), width(w), height(h), needsUpdate(true), cachedTexture(nullptr) {
    // RGBA 포맷의 surface 생성
    surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) {
        SDL_Log("ImageRenderer: Failed to create surface: %s", SDL_GetError());
    }
}

ImageRenderer::~ImageRenderer() {
    if (cachedTexture) {
        SDL_DestroyTexture(cachedTexture);
    }
    if (surface) {
        SDL_FreeSurface(surface);
    }
}

void ImageRenderer::lockSurface() {
    if (SDL_MUSTLOCK(surface)) {
        SDL_LockSurface(surface);
    }
}

void ImageRenderer::unlockSurface() {
    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }
}

void ImageRenderer::updateTexture() {
    if (cachedTexture) {
        SDL_DestroyTexture(cachedTexture);
        cachedTexture = nullptr;
    }
    
    if (surface && renderer) {
        cachedTexture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!cachedTexture) {
            SDL_Log("ImageRenderer: Failed to create texture: %s", SDL_GetError());
        } else {
            // 투명도 유지를 위한 블렌딩 모드 설정
            SDL_SetTextureBlendMode(cachedTexture, SDL_BLENDMODE_BLEND);
        }
        needsUpdate = false;
    }
}

void ImageRenderer::drawPixel(int x, int y, SDL_Color color) {
    if (!surface || x < 0 || x >= width || y < 0 || y >= height) {
        return;
    }
    
    lockSurface();
    
    Uint32* pixels = (Uint32*)surface->pixels;
    Uint32 pixelColor = SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
    pixels[y * surface->pitch / 4 + x] = pixelColor;
    
    unlockSurface();
    needsUpdate = true;
}

void ImageRenderer::drawColor(SDL_Color color) {
    if (!surface) return;
    
    SDL_Rect rect = {0, 0, width, height};
    SDL_FillRect(surface, &rect, SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a));
    needsUpdate = true;
}

void ImageRenderer::drawText(const std::string& text, int x, int y, int fontSize, SDL_Color color, TextRenderer* textRenderer, int maxWidth) {
    if (!surface || !textRenderer || !renderer) return;
    
    // TextRenderer로 텍스처 생성
    SDL_Texture* textTexture = textRenderer->renderText(renderer, text, fontSize, color, maxWidth);
    if (!textTexture) return;
    
    // 텍스처 크기 가져오기
    int texW, texH;
    SDL_QueryTexture(textTexture, nullptr, nullptr, &texW, &texH);
    
    // 임시 렌더 타겟 생성 (텍스처를 surface로 변환하기 위해)
    SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
    
    // 텍스처를 임시 surface로 변환하기 위해 렌더 타겟을 텍스처로 설정
    // 하지만 더 나은 방법: 임시 렌더 타겟 surface 생성
    SDL_Surface* textSurface = SDL_CreateRGBSurfaceWithFormat(0, texW, texH, 32, SDL_PIXELFORMAT_RGBA32);
    if (!textSurface) {
        SDL_DestroyTexture(textTexture);
        return;
    }
    
    // 텍스처를 임시 렌더 타겟으로 렌더링하고 읽기
    SDL_Texture* tempTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, texW, texH);
    if (tempTarget) {
        SDL_SetRenderTarget(renderer, tempTarget);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, textTexture, nullptr, nullptr);
        SDL_RenderReadPixels(renderer, nullptr, textSurface->format->format, textSurface->pixels, textSurface->pitch);
        SDL_SetRenderTarget(renderer, oldTarget);
        SDL_DestroyTexture(tempTarget);
    } else {
        // 실패 시 기존 타겟 복원
        SDL_SetRenderTarget(renderer, oldTarget);
        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);
        return;
    }
    
    // surface에 blit
    SDL_Rect dstRect = {x, y, texW, texH};
    SDL_BlitSurface(textSurface, nullptr, surface, &dstRect);
    
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
    needsUpdate = true;
}

void ImageRenderer::drawRect(int x, int y, int w, int h, SDL_Color color, bool filled) {
    if (!surface) return;
    
    // 범위 체크
    if (x >= width || y >= height || x + w < 0 || y + h < 0) {
        return;
    }
    
    // 클리핑
    int drawX = std::max(0, x);
    int drawY = std::max(0, y);
    int drawW = std::min(w, width - drawX);
    int drawH = std::min(h, height - drawY);
    
    SDL_Rect rect = {drawX, drawY, drawW, drawH};
    Uint32 pixelColor = SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
    
    if (filled) {
        SDL_FillRect(surface, &rect, pixelColor);
    } else {
        // 테두리만 그리기
        lockSurface();
        Uint32* pixels = (Uint32*)surface->pixels;
        int pitch = surface->pitch / 4;
        
        // 상단/하단 테두리
        for (int px = drawX; px < drawX + drawW; px++) {
            if (px >= 0 && px < width) {
                if (drawY >= 0 && drawY < height) {
                    pixels[drawY * pitch + px] = pixelColor;
                }
                if (drawY + drawH - 1 >= 0 && drawY + drawH - 1 < height) {
                    pixels[(drawY + drawH - 1) * pitch + px] = pixelColor;
                }
            }
        }
        
        // 좌측/우측 테두리
        for (int py = drawY; py < drawY + drawH; py++) {
            if (py >= 0 && py < height) {
                if (drawX >= 0 && drawX < width) {
                    pixels[py * pitch + drawX] = pixelColor;
                }
                if (drawX + drawW - 1 >= 0 && drawX + drawW - 1 < width) {
                    pixels[py * pitch + drawX + drawW - 1] = pixelColor;
                }
            }
        }
        
        unlockSurface();
    }
    
    needsUpdate = true;
}

void ImageRenderer::drawCircle(int centerX, int centerY, int radius, SDL_Color color, bool filled) {
    if (!surface || radius <= 0) return;
    
    lockSurface();
    Uint32* pixels = (Uint32*)surface->pixels;
    int pitch = surface->pitch / 4;
    Uint32 pixelColor = SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
    
    if (filled) {
        // 채워진 원 그리기
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x * x + y * y <= radius * radius) {
                    int px = centerX + x;
                    int py = centerY + y;
                    if (px >= 0 && px < width && py >= 0 && py < height) {
                        pixels[py * pitch + px] = pixelColor;
                    }
                }
            }
        }
    } else {
        // 테두리만 그리기 (Bresenham 알고리즘 변형)
        int x = radius;
        int y = 0;
        int err = 0;
        
        while (x >= y) {
            // 8개 대칭점 그리기
            auto drawCirclePoints = [&](int cx, int cy, int px, int py) {
                if (cx + px >= 0 && cx + px < width && cy + py >= 0 && cy + py < height) {
                    pixels[(cy + py) * pitch + (cx + px)] = pixelColor;
                }
                if (cx - px >= 0 && cx - px < width && cy + py >= 0 && cy + py < height) {
                    pixels[(cy + py) * pitch + (cx - px)] = pixelColor;
                }
                if (cx + px >= 0 && cx + px < width && cy - py >= 0 && cy - py < height) {
                    pixels[(cy - py) * pitch + (cx + px)] = pixelColor;
                }
                if (cx - px >= 0 && cx - px < width && cy - py >= 0 && cy - py < height) {
                    pixels[(cy - py) * pitch + (cx - px)] = pixelColor;
                }
                if (cx + py >= 0 && cx + py < width && cy + px >= 0 && cy + px < height) {
                    pixels[(cy + px) * pitch + (cx + py)] = pixelColor;
                }
                if (cx - py >= 0 && cx - py < width && cy + px >= 0 && cy + px < height) {
                    pixels[(cy + px) * pitch + (cx - py)] = pixelColor;
                }
                if (cx + py >= 0 && cx + py < width && cy - px >= 0 && cy - px < height) {
                    pixels[(cy - px) * pitch + (cx + py)] = pixelColor;
                }
                if (cx - py >= 0 && cx - py < width && cy - px >= 0 && cy - px < height) {
                    pixels[(cy - px) * pitch + (cx - py)] = pixelColor;
                }
            };
            
            drawCirclePoints(centerX, centerY, x, y);
            
            if (err <= 0) {
                y += 1;
                err += 2 * y + 1;
            }
            if (err > 0) {
                x -= 1;
                err -= 2 * x + 1;
            }
        }
    }
    
    unlockSurface();
    needsUpdate = true;
}

void ImageRenderer::drawImage(SDL_Surface* srcSurface, int dstX, int dstY, int srcX, int srcY, int srcW, int srcH) {
    if (!surface || !srcSurface) return;
    
    SDL_Rect srcRect;
    if (srcW < 0 || srcH < 0) {
        srcRect = {srcX, srcY, srcSurface->w - srcX, srcSurface->h - srcY};
    } else {
        srcRect = {srcX, srcY, srcW, srcH};
    }
    
    SDL_Rect dstRect = {dstX, dstY, srcRect.w, srcRect.h};
    
    SDL_BlitSurface(srcSurface, &srcRect, surface, &dstRect);
    needsUpdate = true;
}

void ImageRenderer::drawImage(SDL_Texture* srcTexture, int dstX, int dstY, int srcX, int srcY, int srcW, int srcH) {
    if (!surface || !srcTexture || !renderer) {
        return;
    }
    
    // 텍스처 크기 가져오기
    int texW, texH;
    SDL_QueryTexture(srcTexture, nullptr, nullptr, &texW, &texH);
    
    if (srcW < 0) srcW = texW - srcX;
    if (srcH < 0) srcH = texH - srcY;
    
    // 텍스처를 임시 surface로 변환
    SDL_Surface* tempSurface = SDL_CreateRGBSurfaceWithFormat(0, texW, texH, 32, SDL_PIXELFORMAT_RGBA32);
    if (!tempSurface) {
        return;
    }
    
    // 텍스처를 임시 렌더 타겟으로 렌더링하고 읽기
    SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
    SDL_Texture* tempTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, texW, texH);
    if (tempTarget) {
        SDL_SetRenderTarget(renderer, tempTarget);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, srcTexture, nullptr, nullptr);
        SDL_RenderReadPixels(renderer, nullptr, tempSurface->format->format, tempSurface->pixels, tempSurface->pitch);
        SDL_SetRenderTarget(renderer, oldTarget);
        SDL_DestroyTexture(tempTarget);
    } else {
        SDL_SetRenderTarget(renderer, oldTarget);
        SDL_FreeSurface(tempSurface);
        return;
    }
    
    // 부분 영역만 blit
    SDL_Rect srcRect = {srcX, srcY, srcW, srcH};
    SDL_Rect dstRect = {dstX, dstY, srcW, srcH};
    
    SDL_BlitSurface(tempSurface, &srcRect, surface, &dstRect);
    SDL_FreeSurface(tempSurface);
    needsUpdate = true;
}

void ImageRenderer::drawImageScaled(SDL_Texture* srcTexture, int dstX, int dstY, int dstW, int dstH, int srcX, int srcY, int srcW, int srcH) {
    if (!surface || !srcTexture || !renderer) return;

    int texW, texH;
    SDL_QueryTexture(srcTexture, nullptr, nullptr, &texW, &texH);
    if (srcW < 0) srcW = texW - srcX;
    if (srcH < 0) srcH = texH - srcY;

    SDL_Surface* tempSurface = SDL_CreateRGBSurfaceWithFormat(0, texW, texH, 32, SDL_PIXELFORMAT_RGBA32);
    if (!tempSurface) return;

    SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
    SDL_Texture* tempTarget = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, texW, texH);
    if (tempTarget) {
        SDL_SetRenderTarget(renderer, tempTarget);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, srcTexture, nullptr, nullptr);
        SDL_RenderReadPixels(renderer, nullptr, tempSurface->format->format, tempSurface->pixels, tempSurface->pitch);
        SDL_SetRenderTarget(renderer, oldTarget);
        SDL_DestroyTexture(tempTarget);
    } else {
        SDL_SetRenderTarget(renderer, oldTarget);
        SDL_FreeSurface(tempSurface);
        return;
    }

    SDL_Rect srcRect = {srcX, srcY, srcW, srcH};
    SDL_Rect dstRect = {dstX, dstY, dstW, dstH};
    SDL_BlitScaled(tempSurface, &srcRect, surface, &dstRect);
    SDL_FreeSurface(tempSurface);
    needsUpdate = true;
}

SDL_Texture* ImageRenderer::getTexture() {
    if (needsUpdate || !cachedTexture) {
        updateTexture();
    }
    return cachedTexture;
}

void ImageRenderer::clear(SDL_Color color) {
    drawColor(color);
}

