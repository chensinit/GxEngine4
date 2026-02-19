#include "luaScriptExecutor.h"
#include <iostream>

LuaScriptExecutor::LuaScriptExecutor() {
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
}

bool LuaScriptExecutor::loadScriptFromString(const std::string& code) {
    try {
        lua.script(code);
        return true;
    } catch (const sol::error& e) {
        std::cerr << "[Lua] Script error: " << e.what() << std::endl;
        return false;
    }
}



sol::state& LuaScriptExecutor::getState() {
    return lua;
}
