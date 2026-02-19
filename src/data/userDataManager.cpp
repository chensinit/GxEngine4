#include "userDataManager.h"
#include "../resource/resourceManager.h"
#include "../utils/logger.h"
#include "../utils/FileIO.h"

UserDataManager::UserDataManager(ResourceManager* resMgr, const std::string& dataDir)
    : resourceManager(resMgr), dataDir(dataDir) {
    // 데이터 디렉토리 생성 (FileIO가 플랫폼별로 처리)
    if (!FileIO::createDirectoriesForFile(dataDir + "/.keep")) {
        Log::error("[UserDataManager] Failed to create data directory: ", dataDir);
    }
}

std::string UserDataManager::getDataFilePath(const std::string& schemaName) {
    return dataDir + "/" + schemaName + ".json";
}

nlohmann::json UserDataManager::loadSchema(const std::string& schemaName) {
    if (!resourceManager) {
        Log::error("[UserDataManager] ResourceManager is null");
        return nlohmann::json();
    }
    
    nlohmann::json schema = resourceManager->getJson(schemaName);
    if (schema.is_null()) {
        Log::error("[UserDataManager] Schema not found: ", schemaName);
        return nlohmann::json();
    }
    
    return schema;
}

nlohmann::json UserDataManager::createDefaultData(const nlohmann::json& schema) {
    nlohmann::json data = nlohmann::json::object();
    
    for (auto it = schema.begin(); it != schema.end(); ++it) {
        const std::string& key = it.key();
        const nlohmann::json& fieldDef = it.value();
        
        if (fieldDef.contains("default")) {
            data[key] = fieldDef["default"];
        } else {
            // 기본값이 없으면 타입에 따라 기본값 설정
            std::string type = fieldDef.value("type", "string");
            if (type == "number") {
                data[key] = 0;
            } else if (type == "string") {
                data[key] = "";
            } else if (type == "boolean") {
                data[key] = false;
            } else if (type == "table") {
                data[key] = nlohmann::json::array();
            }
        }
    }
    
    return data;
}

nlohmann::json UserDataManager::loadDataFile(const std::string& schemaName, 
                                              const nlohmann::json& schema) {
    std::string dataPath = getDataFilePath(schemaName);
    std::string content = FileIO::readFileAsText(dataPath);
    
    if (content.empty()) {
        return createDefaultData(schema);
    }
    
    try {
        nlohmann::json data = nlohmann::json::parse(content);
        
        // 스키마에 있는 필드만 유지하고, 없는 필드는 기본값 추가
        nlohmann::json result = createDefaultData(schema);
        for (auto it = data.begin(); it != data.end(); ++it) {
            if (schema.contains(it.key())) {
                result[it.key()] = it.value();
            }
        }
        
        return result;
    } catch (const std::exception& e) {
        Log::error("[UserDataManager] Failed to parse data file: ", e.what());
        return createDefaultData(schema);
    }
}

nlohmann::json UserDataManager::luaToJson(sol::object luaObj) {
    if (luaObj.get_type() == sol::type::number) {
        double val = luaObj.as<double>();
        // 정수인지 확인
        if (val == static_cast<int64_t>(val)) {
            return static_cast<int64_t>(val);
        }
        return val;
    } else if (luaObj.get_type() == sol::type::string) {
        return luaObj.as<std::string>();
    } else if (luaObj.get_type() == sol::type::boolean) {
        return luaObj.as<bool>();
    } else if (luaObj.get_type() == sol::type::table) {
        sol::table tbl = luaObj.as<sol::table>();
        nlohmann::json result;
        
        // 배열인지 객체인지 확인
        bool isArray = true;
        int maxIndex = 0;
        for (auto pair : tbl) {
            if (pair.first.get_type() != sol::type::number) {
                isArray = false;
                break;
            }
            int idx = pair.first.as<int>();
            if (idx > maxIndex) maxIndex = idx;
        }
        
        if (isArray && maxIndex > 0) {
            // 배열
            result = nlohmann::json::array();
            for (int i = 1; i <= maxIndex; ++i) {
                sol::object val = tbl[i];
                result.push_back(luaToJson(val));
            }
        } else {
            // 객체
            result = nlohmann::json::object();
            for (auto pair : tbl) {
                std::string key = pair.first.as<std::string>();
                result[key] = luaToJson(pair.second);
            }
        }
        
        return result;
    }
    
    return nlohmann::json();
}

sol::table UserDataManager::jsonToLua(sol::state_view& lua, const nlohmann::json& json) {
    sol::table result = lua.create_table();
    
    if (json.is_object()) {
        for (auto it = json.begin(); it != json.end(); ++it) {
            const auto& value = it.value();
            if (value.is_number_integer()) {
                result[it.key()] = value.get<int64_t>();
            } else if (value.is_number_float()) {
                result[it.key()] = value.get<double>();
            } else if (value.is_string()) {
                result[it.key()] = value.get<std::string>();
            } else if (value.is_boolean()) {
                result[it.key()] = value.get<bool>();
            } else if (value.is_array() || value.is_object()) {
                result[it.key()] = jsonToLua(lua, value);
            }
        }
    } else if (json.is_array()) {
        for (size_t i = 0; i < json.size(); ++i) {
            const auto& value = json[i];
            if (value.is_number_integer()) {
                result[i + 1] = value.get<int64_t>();  // Lua는 1-based
            } else if (value.is_number_float()) {
                result[i + 1] = value.get<double>();
            } else if (value.is_string()) {
                result[i + 1] = value.get<std::string>();
            } else if (value.is_boolean()) {
                result[i + 1] = value.get<bool>();
            } else if (value.is_array() || value.is_object()) {
                result[i + 1] = jsonToLua(lua, value);
            }
        }
    }
    
    return result;
}

sol::table UserDataManager::load(const std::string& schemaName, sol::state_view& lua) {
    // 이미 로드된 인스턴스가 있으면 재사용
    if (loadedInstances.find(schemaName) != loadedInstances.end()) {
        auto& instance = loadedInstances[schemaName];
        return jsonToLua(lua, instance.data);
    }
    
    // 스키마 로드
    nlohmann::json schema = loadSchema(schemaName);
    if (schema.is_null() || schema.empty()) {
        Log::error("[UserDataManager] Failed to load schema: ", schemaName);
        return lua.create_table();
    }
    
    // 데이터 로드
    nlohmann::json data = loadDataFile(schemaName, schema);
    
    // 인스턴스 저장
    DataInstance instance;
    instance.schemaName = schemaName;
    instance.schema = schema;
    instance.data = data;
    instance.isDirty = false;
    instance.dataPath = getDataFilePath(schemaName);
    
    loadedInstances[schemaName] = instance;
    
    // Lua 테이블로 변환
    sol::table result = jsonToLua(lua, data);
    
    // save 메서드 추가
    UserDataManager* manager = this;
    std::string name = schemaName;
    result["save"] = [manager, name]() {
        manager->save(name);
    };
    
    // 메타테이블로 값 변경 감지
    sol::table metatable = lua.create_table();
    metatable["__newindex"] = [manager, name](sol::table self, 
                                               const std::string& key, 
                                               sol::object value) {
        try {
            auto& instance = manager->loadedInstances[name];
            instance.data[key] = manager->luaToJson(value);
            instance.isDirty = true;
            self[key] = value;
        } catch (const std::exception& e) {
            Log::error("[UserDataManager] Error in __newindex: ", e.what());
            throw;
        }
    };
    
    result[sol::metatable_key] = metatable;
    
    return result;
}

bool UserDataManager::save(const std::string& schemaName) {
    if (loadedInstances.find(schemaName) == loadedInstances.end()) {
        Log::error("[UserDataManager] Instance not found: ", schemaName);
        return false;
    }
    
    auto& instance = loadedInstances[schemaName];
    std::string dataPath = instance.dataPath;
    
    if (!FileIO::writeFileAsText(dataPath, instance.data.dump(4))) {
        Log::error("[UserDataManager] Failed to open file for writing: ", dataPath);
        return false;
    }
    
    instance.isDirty = false;
    return true;
}

bool UserDataManager::saveFromLuaTable(const std::string& schemaName, sol::table luaTable) {
    if (loadedInstances.find(schemaName) == loadedInstances.end()) {
        Log::error("[UserDataManager] Instance not found: ", schemaName);
        return false;
    }
    
    auto& instance = loadedInstances[schemaName];
    
    // Lua 테이블의 모든 값을 다시 읽어서 JSON으로 변환
    for (auto pair : luaTable) {
        std::string key = pair.first.as<std::string>();
        instance.data[key] = luaToJson(pair.second);
    }
    
    instance.isDirty = true;
    return save(schemaName);
}

sol::table UserDataManager::jsonToLuaTable(sol::state_view& lua, const nlohmann::json& json) {
    return jsonToLua(lua, json);
}

sol::table UserDataManager::getJsonAsLuaTable(sol::state_view& lua, const std::string& jsonName) {
    if (!resourceManager) {
        Log::error("[UserDataManager] ResourceManager is null");
        return lua.create_table();
    }
    
    nlohmann::json json = resourceManager->getJson(jsonName);
    if (json.is_null()) {
        return lua.create_table();
    }
    
    return jsonToLua(lua, json);
}

