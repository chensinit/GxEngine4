#pragma once

#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>

class ResourceManager;  // forward declaration

class UserDataManager {
private:
    struct DataInstance {
        std::string schemaName;
        nlohmann::json schema;
        nlohmann::json data;
        bool isDirty;
        std::string dataPath;
    };
    
    std::map<std::string, DataInstance> loadedInstances;
    ResourceManager* resourceManager;
    std::string dataDir;        // savedata/
    
    // 스키마 로드 (ResourceManager를 통해)
    nlohmann::json loadSchema(const std::string& schemaName);
    
    // 데이터 파일 경로 생성
    std::string getDataFilePath(const std::string& schemaName);
    
    // 데이터 로드 (파일이 없으면 기본값으로 생성)
    nlohmann::json loadDataFile(const std::string& schemaName, 
                                 const nlohmann::json& schema);
    
    // 기본값으로 데이터 초기화
    nlohmann::json createDefaultData(const nlohmann::json& schema);
    
    // Lua 타입을 nlohmann::json으로 변환
    nlohmann::json luaToJson(sol::object luaObj);
    
    // nlohmann::json을 Lua 테이블로 변환
    sol::table jsonToLua(sol::state_view& lua, const nlohmann::json& json);
    
public:
    UserDataManager(ResourceManager* resMgr, const std::string& dataDir);
    
    // 스키마 이름으로 데이터 로드
    sol::table load(const std::string& schemaName, sol::state_view& lua);
    
    // 데이터 저장
    bool save(const std::string& schemaName);
    
    // Lua 테이블에서 데이터를 다시 읽어서 저장 (메타테이블이 작동하지 않을 때 사용)
    bool saveFromLuaTable(const std::string& schemaName, sol::table luaTable);
    
    // JSON을 Lua 테이블로 변환 (읽기 전용 데이터 로드용)
    sol::table jsonToLuaTable(sol::state_view& lua, const nlohmann::json& json);
    
    // ResourceManager를 통해 JSON 파일을 가져와서 Lua 테이블로 변환
    sol::table getJsonAsLuaTable(sol::state_view& lua, const std::string& jsonName);
};

