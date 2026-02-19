#pragma once
#include <sol/sol.hpp>
#include <string>
#include <iostream>

class LuaScriptExecutor {
public:
    LuaScriptExecutor();

    bool loadScriptFromString(const std::string& code);
    template<typename... Args>
    bool callFunction(const std::string& funcName, Args&&... args);

    sol::state& getState();

private:
    sol::state lua;
};

template<typename... Args>
bool LuaScriptExecutor::callFunction(const std::string& funcName, Args&&... args) {
    sol::function func = lua[funcName];
    if (!func.valid()) {
        std::cerr << "[Lua] Function not found: " << funcName << std::endl;
        return false;
    }

    try {
        func(std::forward<Args>(args)...);
        return true;
    } catch (const sol::error& e) {
        std::cerr << "[Lua] Runtime error in " << funcName << ": " << e.what() << std::endl;
        return false;
    }
}
