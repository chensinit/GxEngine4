#include "scene.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <memory>


using json = nlohmann::json;

Scene::Scene(SDL_Renderer* sdlRenderer, ResourceManager* resourceMgr)
    : renderer(sdlRenderer), resourceManager(resourceMgr),
      widgetManager(&uiManager, resourceMgr, sdlRenderer, &textRenderer, &animationManager),
      userDataManager(resourceMgr, "savedata"),
      pendingSceneName("") {
        // init classes
        uiManager.setResourceManager(resourceManager);
        animationManager.setUiManager(&uiManager);
        textRenderer.init("data/Jalnan2.otf");
        
        scriptManager.setLoadScene([this](const std::string& name) {
            requestSceneChange(name);  // 즉시 전환하지 않고 요청만 저장
        });
        scriptManager.setUiManager(&uiManager);
        scriptManager.setWidgetManager(&widgetManager);
        scriptManager.setAnimationManager(&animationManager);
        scriptManager.setUserDataManager(&userDataManager);
        scriptManager.setResourceManager(resourceManager);
        animationManager.setResourceManager(resourceManager);
    }

void Scene::loadScene(const std::string& sceneName) {
    // drawList.clear(); // 기존 데이터 초기화

    auto sceneData = resourceManager->getSceneJson(sceneName);
    if (sceneData.is_null()) {
        Log::error("Scene not found or invalid: ", sceneName);
        return;
    }

    try {
        // 씬 전환 시 리소스 릴리즈 (순서 중요!)
        // 1. 애니메이션 제거 (UIElement 참조하므로 UIElement 제거 전)
        animationManager.clear();
        
        // 2. 위젯 제거 (소멸자에서 자신의 텍스처 해제)
        // 위젯 소멸자가 UIElement를 참조하므로, 위젯 제거 전에 UIElement가 있어야 함
        // 위젯의 Lua 콜백이 무효화되기 전에 위젯을 먼저 제거해야 함
        widgetManager.clear();
        
        // 3. UIElement 제거 (위젯이 정리한 후)
        uiManager.clear();
        
        // 4. 스크립트 제거 (위젯이 제거된 후, Lua 상태 파괴)
        // 위젯의 Lua 콜백이 무효화되기 전에 위젯을 제거했으므로 안전함
        scriptManager.reset();
        
        // 5. Texture 제거는 위젯 소멸자에서 처리됨
        // (ResourceManager 참조 카운팅으로 자동 해제)
    
        // ui 배열을 순회하며, include_layout 타입이 있으면 그 자리에서 레이아웃 ui를 끼워 넣은 리스트 구성
        json uiList = json::array();
        if (sceneData.contains("ui") && sceneData["ui"].is_array()) {
            for (const auto& item : sceneData["ui"]) {
                std::string type = item.contains("type") ? item["type"].get<std::string>() : "";
                if (type == "include_layout" && item.contains("layout")) {
                    std::string layoutName = item["layout"].get<std::string>();
                    auto layoutData = resourceManager->getSceneJson(layoutName);
                    if (!layoutData.is_null() && layoutData.contains("ui") && layoutData["ui"].is_array()) {
                        for (const auto& uiElement : layoutData["ui"]) {
                            uiList.push_back(uiElement);
                        }
                    } else {
                        Log::error("[Scene] include_layout: layout not found or has no ui: ", layoutName);
                    }
                } else {
                    uiList.push_back(item);
                }
            }
        }

        for (const auto& uiElement : uiList) {
            std::string type = uiElement["type"].get<std::string>();
            std::string name = uiElement.contains("name") ? uiElement["name"].get<std::string>() : "unnamed";

            // image 타입은 UiManager로
            if (type == "image") {
                uiManager.loadUIFromJson(uiElement, renderer, &textRenderer);
            }
            // text, button 등 위젯 타입은 WidgetManager로
            else {
                bool success = widgetManager.loadWidgetFromJson(uiElement);
                if (!success) {
                    Log::error("[Scene] Failed to load widget: ", name, " (type: ", type, ")");
                }
            }
        }
    } catch (const std::exception& e) {
        Log::error("JSON parsing error: ", e.what());
    }

    if (sceneData.contains("code")) {
        std::string codeName = sceneData["code"].get<std::string>();
        std::string codeContent = resourceManager->getText(codeName);
        if (!codeContent.empty()) {
            scriptManager.loadScript(codeContent);
            scriptManager.call("init");  // 초기화 함수 호출
        } else {
            Log::error("Code script \"", codeName, "\" not found.");
        }
    }

//test
// auto anim = std::make_shared<Animator>("right");

// anim->rotate(0.0f, 360.0f, 1000).
//      repeat(-1).
//      callback([]() {
//          std::cout << "회전 애니메이션 완료!" << std::endl;
//      });
// animationManager.add("right", anim);
}

void Scene::requestSceneChange(const std::string& sceneName) {
    // 즉시 전환하지 않고 다음 프레임에 전환하도록 저장
    pendingSceneName = sceneName;
    Log::info("[Scene] Scene change requested: ", sceneName, " (will load next frame)");
}

void Scene::processPendingSceneChange() {
    if (!pendingSceneName.empty()) {
        std::string sceneToLoad = pendingSceneName;
        pendingSceneName = "";  // 초기화 (재귀 호출 방지)
        Log::info("[Scene] Processing pending scene change: ", sceneToLoad);
        loadScene(sceneToLoad);
    }
}


// todo: It should be moved TouchManager or Touch Util
bool mouseDown = false;
int mouseDownX = 0, mouseDownY = 0;

// SDL_USER 이벤트 타입 등록 (mouse_click용)
Uint32 MOUSE_CLICK_EVENT = 0;

void Scene::keyPressed(const std::vector<SDL_Event>& events) {
    // SDL_USER 이벤트 타입 등록 (한 번만)
    if (MOUSE_CLICK_EVENT == 0) {
        MOUSE_CLICK_EVENT = SDL_RegisterEvents(1);
        if (MOUSE_CLICK_EVENT == SDL_USEREVENT) {
            // 등록 성공
        }
    }
    
    // 1단계: click 감지 및 SDL_USER 이벤트 생성
    std::vector<SDL_Event> eventsForWidget;
    
    for (const auto& event : events) {
        eventsForWidget.push_back(event);
        
        // mouse_up에서 click 감지
        if (event.type == SDL_MOUSEBUTTONUP) {
            int x = event.button.x;
            int y = event.button.y;
            
            // check click
            int dx = x - mouseDownX;
            int dy = y - mouseDownY;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < 10) {
                // click이 감지된 경우 SDL_USER 이벤트 생성
                SDL_Event clickEvent;
                clickEvent.type = MOUSE_CLICK_EVENT;
                clickEvent.user.timestamp = event.button.timestamp;
                clickEvent.user.windowID = event.button.windowID;
                clickEvent.user.code = event.button.button;  // 버튼 번호
                // 좌표를 data1, data2에 저장 (포인터로)
                int* coords = new int[2];
                coords[0] = x;
                coords[1] = y;
                clickEvent.user.data1 = coords;
                clickEvent.user.data2 = nullptr;
                eventsForWidget.push_back(clickEvent);
            }
        }
        
        // mouse_down에서 좌표 저장
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            mouseDownX = event.button.x;
            mouseDownY = event.button.y;
            mouseDown = true;
        }
        
        if (event.type == SDL_MOUSEBUTTONUP) {
            mouseDown = false;
        }
    }
    
    // 2단계: 위젯 이벤트 처리
    widgetManager.handleEvents(eventsForWidget);
    
    // 3단계: Lua로 이벤트 전달
    UIWidget* focusedWidget = widgetManager.getFocusedWidget();
    
    for (const auto& event : eventsForWidget) {
        // 마우스 관련 이벤트와 키 입력만 Lua로 전달
        bool isMouseEvent = (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP || 
                            event.type == SDL_MOUSEMOTION || event.type == MOUSE_CLICK_EVENT);
        bool isKeyEvent = (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP);
        
        // 마우스/키 이벤트가 아니면 Lua로 전달하지 않음
        if (!isMouseEvent && !isKeyEvent) {
            // SDL_USER 이벤트의 메모리 해제
            if (event.type == MOUSE_CLICK_EVENT && event.user.data1) {
                delete[] static_cast<int*>(event.user.data1);
            }
            continue;
        }
        
        // 포커스가 있는 위젯이 있고, 키보드 이벤트면 Lua로 전달하지 않음 (단축키 제외)
        if (focusedWidget && isKeyEvent) {
            SDL_Keycode keycode = event.key.keysym.sym;
            Uint16 mod = event.key.keysym.mod;
            
            // Ctrl/Cmd/Alt 조합이 있으면 단축키로 전달
            if (!(mod & (KMOD_CTRL | KMOD_GUI | KMOD_ALT))) {
                // 일반 키는 위젯이 처리했을 수 있으므로 제외
                // SDL_USER 이벤트의 메모리 해제
                if (event.type == MOUSE_CLICK_EVENT && event.user.data1) {
                    delete[] static_cast<int*>(event.user.data1);
                }
                continue;
            }
        }
        
        // 필터링된 이벤트만 ScriptManager로 전달
        scriptManager.handleEvent(event, MOUSE_CLICK_EVENT);
    }
}

void Scene::update(float deltaTime) {
    // 대기 중인 씬 전환 처리 (다른 업데이트보다 먼저)
    processPendingSceneChange();
    
    widgetManager.update(deltaTime);  // 위젯 업데이트
    animationManager.update(deltaTime);
    scriptManager.call("update");
}


void Scene::render() {
    // UIElement 렌더링 (UiManager에서 처리, 자식은 부모 영역으로 클리핑됨)
    uiManager.render(renderer);
    
    // 위젯 특수 렌더링 (필요시만)
    widgetManager.render(renderer);
}
