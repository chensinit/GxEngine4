#pragma once

#include <string>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include "../ads/StubAdProvider.h"
#include "../utils/sdl_includes.h"
#include "luaScriptExecutor.h"  // 같은 폴더
#include "../ui/uiManager.h"
#include "../animation/AnimationManager.h"

// class UiManager;

class WidgetManager;  // forward declaration
class UserDataManager;  // forward declaration
class ResourceManager;  // forward declaration

class ScriptManager {
public:
    ScriptManager();
    void loadScript(const std::string& code);
    void setCommonApi();
    void setUiManager(UiManager* ui);
    void setWidgetManager(WidgetManager* wm);
    void setAnimationManager(AnimationManager* am);
    void setUserDataManager(UserDataManager* udm);
    void setResourceManager(ResourceManager* rm);
    template<typename... Args>
    void call(const std::string& functionName, Args&&... args);
    void reset();

    // loadScene from scene.h
    using LoadSceneFn = std::function<void(const std::string&)>;
    void setLoadScene(LoadSceneFn fn);
    void loadScene(const std::string name);

    
    // SDL 이벤트를 받아서 Lua로 전달 (이미 필터링된 이벤트만 전달됨)
    void handleEvent(const SDL_Event& event, Uint32 mouseClickEvent);

private:
    void setUserDataApi();  // UserData API 바인딩
    void setResourceApi();  // Resource API 바인딩
    
    LuaScriptExecutor executor;
    UiManager* uiManager = nullptr;
    WidgetManager* widgetManager = nullptr;
    AnimationManager* animationManager = nullptr;
    UserDataManager* userDataManager = nullptr;
    ResourceManager* resourceManager = nullptr;

    LoadSceneFn loadSceneFn;

    std::map<std::string, std::string> globalData;  // Lua에서 어디서든 접근 가능한 키-값 저장소

    std::unique_ptr<StubAdProvider> adProvider;

    nlohmann::json luaObjectToJson(sol::object obj);
    sol::table jsonToLuaTable(sol::state_view& lua, const nlohmann::json& json);
};

template<typename... Args>
void ScriptManager::call(const std::string& funcName, Args&&... args){
    executor.callFunction(funcName, std::forward<Args>(args)...);
}