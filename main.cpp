#include "main.h"
#include "src/scene.h"
#include "src/resource/resourceManager.h"
#include "src/utils/logger.h"
#include "src/utils/FileIO.h"
#include "src/utils/sdl_includes.h"
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

bool loadSettings(const std::string &filePath, int &width, int &height, int &windowWidth, int &windowHeight, std::string &resourceFile) {
    std::string content = FileIO::readFileAsText(filePath);
    if (content.empty()) {
        Log::error("[Main] Failed to open settings file: ", filePath);
        return false;
    }
    
    try {
        nlohmann::json settings = nlohmann::json::parse(content);
        
        // 내부 렌더링 해상도
        width = settings.value("width", 800);
        height = settings.value("height", 600);
        
        // 실제 창 크기 (지정하지 않으면 내부 해상도와 동일)
        windowWidth = settings.value("window_width", width);
        windowHeight = settings.value("window_height", height);
        
        resourceFile = settings.value("resource_file", "resource.json");
        
        Log::info("[Main] Settings loaded - render: ", width, "x", height, ", window: ", windowWidth, "x", windowHeight, ", resource_file: ", resourceFile);
        return true;
    } catch (const std::exception& e) {
        Log::error("[Main] Error parsing settings file: ", e.what());
        return false;
    }
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Windows에서 콘솔 창 생성
    AllocConsole();
    
    // stdout, stderr을 콘솔에 리다이렉트
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    
    // 콘솔 창 제목 설정
    SetConsoleTitleA("SDL2 Engine Debug Console");
#endif

    // 설정 파일 로드
    int renderWidth = 800;      // 내부 렌더링 해상도
    int renderHeight = 600;
    int windowWidth = 800;      // 실제 창 크기
    int windowHeight = 600;
    std::string resourceFile = "resource.json";
    
    if (!loadSettings("setting.json", renderWidth, renderHeight, windowWidth, windowHeight, resourceFile)) {
        Log::info("[Main] Using default settings");
    }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("App", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // 논리적 해상도 설정 - 게임은 항상 renderWidth x renderHeight로 렌더링하고, 실제 창 크기에 맞춰 자동 스케일링
    SDL_RenderSetLogicalSize(renderer, renderWidth, renderHeight);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");  // 1=linear filtering (부드러운 스케일링)

    SDL_SetHint(SDL_HINT_MOUSE_DOUBLE_CLICK_TIME, "250");  // mouse click time limit
    SDL_SetHint(SDL_HINT_MOUSE_DOUBLE_CLICK_RADIUS, "10"); // mouse click loc limit

    // ResourceManager 생성
    ResourceManager resourceManager(renderer);
    resourceManager.loadResources(resourceFile);

    // Scene 생성 시 ResourceManager 전달
    Scene scene(renderer, &resourceManager);

    // ResourceManager에서 초기 씬 이름 가져오기
    std::string initialScene = resourceManager.getInitialScene();
    if (initialScene.empty()) {
        initialScene = "scene_menu";  // 기본값
        Log::info("[Main] Using default initial scene: scene_menu");
    } else {
        Log::info("[Main] Loading initial scene: ", initialScene);
    }
    scene.loadScene(initialScene);

    using clock = std::chrono::high_resolution_clock;
    auto lastTime = clock::now();

    const float targetFrameTimeMs = 1000.0f / 60.0f;  // ≈ 16.67ms

    bool running = true;
    SDL_Event event;

    while (running) {
        auto frameStart = clock::now();
        std::chrono::duration<float, std::milli> delta = frameStart - lastTime;
        float deltaTimeMs = delta.count();
        
        // 첫 프레임이나 deltaTime이 너무 크면 제한 (예: 100ms 이상이면 16.67ms로 제한)
        if (deltaTimeMs > 100.0f) {
            deltaTimeMs = targetFrameTimeMs;
        }
        
        lastTime = frameStart;

        // 이벤트 처리
        std::vector<SDL_Event> events;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
            else
                events.push_back(event);
        }

        SDL_RenderClear(renderer);

        scene.keyPressed(events);
        scene.update(deltaTimeMs);
        scene.render();
        SDL_RenderPresent(renderer);

        // ⏳ 남은 시간만큼 대기해서 60fps 유지
        auto frameEnd = clock::now();
        std::chrono::duration<float, std::milli> frameDuration = frameEnd - frameStart;
        float frameMs = frameDuration.count();

        if (frameMs < targetFrameTimeMs) {
            SDL_Delay(static_cast<Uint32>(targetFrameTimeMs - frameMs));
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

