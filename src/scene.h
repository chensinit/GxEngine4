#ifndef SCENE_H
#define SCENE_H

#include "utils/sdl_includes.h"
#include <string>
#include <vector>
#include <utility>
#include "resource/resourceManager.h"
#include "scripting/scriptManager.h"
#include "ui/uiManager.h"
#include "animation/AnimationManager.h"
#include "rendering/TextRenderer.h"
#include "animation/Animator.h"
#include "widgets/WidgetManager.h"
#include "data/userDataManager.h"

// SDL_USER 이벤트 타입 (mouse_click용)
extern Uint32 MOUSE_CLICK_EVENT;

class Scene {
private:
    SDL_Renderer* renderer;
    ResourceManager* resourceManager;
    UiManager uiManager;
    ScriptManager scriptManager;  // WidgetManager보다 먼저 선언 → 소멸 시 WidgetManager가 먼저 파괴되어 Lua ref 해제 시 크래시 방지
    WidgetManager widgetManager;
    AnimationManager animationManager;
    TextRenderer textRenderer;
    UserDataManager userDataManager;
    
    std::string pendingSceneName;  // 다음 프레임에 로드할 씬 이름

    // std::vector<std::pair<SDL_Texture*, SDL_Rect>> drawList;
    
    void processPendingSceneChange();  // 대기 중인 씬 전환 처리

public:
    Scene(SDL_Renderer* sdlRenderer, ResourceManager* resourceMgr);
    void loadScene(const std::string& sceneName);
    void requestSceneChange(const std::string& sceneName);  // 씬 전환 요청 (다음 프레임에 처리)
    void keyPressed(const std::vector<SDL_Event>& events);
    void update(float deltaTime);
    void render();
};

#endif
