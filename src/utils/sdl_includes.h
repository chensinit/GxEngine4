#pragma once

/**
 * SDL include 경로 통일 - Android/NDK와 데스크톱(Homebrew/vcpkg) 호환
 * Android NDK: <SDL.h>, <SDL_image.h>, <SDL_ttf.h>
 * 데스크톱:    <SDL2/SDL.h>, <SDL2/SDL_image.h>, <SDL2/SDL_ttf.h>
 */
#ifdef __ANDROID__
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#endif
