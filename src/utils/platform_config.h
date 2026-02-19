#pragma once

/**
 * 플랫폼 상수 (빌드 타겟에 따라 고정, 런타임 변경 없음)
 * 파일 I/O 등 플랫폼별 분기 시 이 상수만 참조하면 됨.
 */
#ifdef __ANDROID__
#define SDL2_ENGINE_PLATFORM_ANDROID 1
#else
#define SDL2_ENGINE_PLATFORM_ANDROID 0
#endif
