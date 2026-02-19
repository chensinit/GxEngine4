cmake_minimum_required(VERSION 3.16)

# macOS SDK 자동 감지 및 설정
if(APPLE)
    execute_process(
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE MACOS_SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(MACOS_SDK_PATH)
        set(CMAKE_OSX_SYSROOT ${MACOS_SDK_PATH} CACHE PATH "macOS SDK path" FORCE)
        message(STATUS "Using macOS SDK: ${CMAKE_OSX_SYSROOT}")
    endif()
    
    # C++ 표준 라이브러리 경로 설정
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isysroot ${CMAKE_OSX_SYSROOT}")
endif()

project(MyGame)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 플랫폼별 설정
if(WIN32)
    # Windows용 설정
    set(CMAKE_SYSTEM_NAME Windows)
    
    # MinGW 64비트 라이브러리 경로 설정
    set(SDL2_ROOT "${CMAKE_SOURCE_DIR}/../libs/SDL2-2.32.10/x86_64-w64-mingw32")
    set(SDL2_IMAGE_ROOT "${CMAKE_SOURCE_DIR}/../libs/SDL2_image-2.8.8/x86_64-w64-mingw32")
    set(SDL2_TTF_ROOT "${CMAKE_SOURCE_DIR}/../libs/SDL2_ttf-2.24.0/x86_64-w64-mingw32")
    
    # CMake가 SDL2를 찾을 수 있도록 경로 설정
    list(APPEND CMAKE_PREFIX_PATH 
        ${SDL2_ROOT}
        ${SDL2_IMAGE_ROOT}/lib/cmake
        ${SDL2_TTF_ROOT}/lib/cmake
    )
    
    # CMP0144 정책 설정 (경고 제거)
    if(POLICY CMP0144)
        cmake_policy(SET CMP0144 NEW)
    endif()
    
    # SDL2 찾기
    find_package(SDL2 REQUIRED CONFIG)
    
    # SDL2_image 찾기
    find_package(SDL2_image REQUIRED CONFIG)
    
    # SDL2_ttf 찾기
    find_package(SDL2_ttf REQUIRED CONFIG)
    
    # DLL 파일을 실행 파일과 같은 디렉토리에 복사하기 위한 설정
    set(SDL2_DLL_DIR "${SDL2_ROOT}/bin")
    set(SDL2_IMAGE_DLL "${SDL2_IMAGE_ROOT}/bin/SDL2_image.dll")
    set(SDL2_TTF_DLL "${SDL2_TTF_ROOT}/bin/SDL2_ttf.dll")
    set(SDL2_DLL "${SDL2_ROOT}/bin/SDL2.dll")
    
elseif(APPLE)
    # macOS용 설정
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)
    find_package(SDL2_ttf REQUIRED)
else()
    # Linux용 설정
    set(CMAKE_EXE_LINKER_FLAGS "-L/lib/x86_64-linux-gnu -L/usr/lib/x86_64-linux-gnu")
    
    find_package(SDL2 REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
endif()

# 다른 라이브러리 찾기 (vcpkg 또는 시스템 패키지)
# vcpkg를 사용하는 경우, 빌드 스크립트에서 CMAKE_TOOLCHAIN_FILE을 지정해야 합니다

# vcpkg가 설치된 경우 CMAKE_PREFIX_PATH에 추가
if(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/installed/x64-mingw-static")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-mingw-static")
    message(STATUS "Added vcpkg path: $ENV{VCPKG_ROOT}/installed/x64-mingw-static")
endif()

# CMAKE_PREFIX_PATH가 이미 설정되어 있다면 사용
if(CMAKE_PREFIX_PATH)
    message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
endif()

# jsoncpp 찾기
find_package(jsoncpp CONFIG QUIET)
if(NOT jsoncpp_FOUND)
    # macOS에서 Homebrew로 설치된 경우
    if(APPLE)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(JSONCPP jsoncpp)
            if(JSONCPP_FOUND)
                add_library(JsonCpp::JsonCpp INTERFACE IMPORTED)
                target_include_directories(JsonCpp::JsonCpp INTERFACE ${JSONCPP_INCLUDE_DIRS})
                target_link_libraries(JsonCpp::JsonCpp INTERFACE ${JSONCPP_LIBRARIES})
                set(jsoncpp_FOUND TRUE)
            endif()
        endif()
    endif()
endif()
if(NOT jsoncpp_FOUND)
    message(WARNING "jsoncpp를 찾을 수 없습니다. vcpkg를 사용하세요.")
    if(WIN32)
        message(STATUS "vcpkg 설치: git clone https://github.com/Microsoft/vcpkg.git")
        message(STATUS "            cd vcpkg")
        message(STATUS "            .\\bootstrap-vcpkg.bat")
        message(STATUS "            .\\vcpkg install jsoncpp:x64-mingw-static")
    else()
        message(STATUS "macOS: brew install jsoncpp")
    endif()
    message(FATAL_ERROR "jsoncpp가 필요합니다.")
endif()

# nlohmann_json 찾기
find_package(nlohmann_json CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
    # macOS에서 Homebrew로 설치된 경우
    if(APPLE)
        find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp
            PATHS /opt/homebrew/include /usr/local/include
        )
        if(NLOHMANN_JSON_INCLUDE_DIR)
            add_library(nlohmann_json INTERFACE IMPORTED)
            target_include_directories(nlohmann_json INTERFACE ${NLOHMANN_JSON_INCLUDE_DIR})
            add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
            set(nlohmann_json_FOUND TRUE)
        endif()
    endif()
endif()
if(NOT nlohmann_json_FOUND)
    message(WARNING "nlohmann_json을 찾을 수 없습니다.")
    if(WIN32)
        message(STATUS "vcpkg 설치: .\\vcpkg install nlohmann-json:x64-mingw-static")
    else()
        message(STATUS "macOS: brew install nlohmann-json")
    endif()
    message(FATAL_ERROR "nlohmann_json이 필요합니다.")
endif()

# sol2 찾기
find_package(sol2 CONFIG QUIET)
if(NOT sol2_FOUND)
    # 프로젝트에 포함된 sol2 사용
    if(EXISTS "${CMAKE_SOURCE_DIR}/third_party/sol2/include")
        message(STATUS "Using bundled sol2 from third_party/sol2")
        add_library(sol2 INTERFACE)
        target_include_directories(sol2 INTERFACE "${CMAKE_SOURCE_DIR}/third_party/sol2/include")
        add_library(sol2::sol2 ALIAS sol2)
        set(sol2_FOUND TRUE)
    else()
        message(WARNING "sol2를 찾을 수 없습니다. vcpkg를 사용하세요.")
        message(STATUS "vcpkg 설치: .\\vcpkg install sol2:x64-mingw-static")
        message(FATAL_ERROR "sol2가 필요합니다. vcpkg를 설치하고 빌드 스크립트에서 VCPKG_ROOT를 설정하세요.")
    endif()
endif()

# lua 찾기
# vcpkg toolchain을 사용하는 경우 자동으로 찾아짐
# lua는 vcpkg에서 제공하는 FindLua.cmake를 사용하거나 직접 찾아야 함
find_package(lua CONFIG QUIET)
if(NOT lua_FOUND)
    # macOS에서 Homebrew로 설치된 lua 찾기
    if(APPLE)
        find_library(LUA_LIBRARY
            NAMES lua lua5.4 lua54
            PATHS /opt/homebrew/lib /usr/local/lib
            NO_DEFAULT_PATH
        )
        if(LUA_LIBRARY)
            add_library(lua UNKNOWN IMPORTED)
            set_target_properties(lua PROPERTIES IMPORTED_LOCATION ${LUA_LIBRARY})
            set(lua_FOUND TRUE)
            message(STATUS "Found lua library: ${LUA_LIBRARY}")
        endif()
    endif()
    
    # vcpkg toolchain을 사용하지 않는 경우 수동으로 찾기
    if(NOT lua_FOUND AND DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/installed/x64-mingw-static")
        # 경로를 슬래시로 변환하여 이스케이프 문제 방지
        file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}/installed/x64-mingw-static" LUA_ROOT)
        # vcpkg는 보통 share/lua 또는 lib/cmake/lua에 CMake 파일을 둠
        find_package(lua CONFIG QUIET PATHS "${LUA_ROOT}/share/lua" "${LUA_ROOT}/lib/cmake/lua" "${LUA_ROOT}")
    endif()
endif()

# lua를 찾지 못한 경우, sol2가 lua를 포함하고 있을 수 있으므로 경고만 표시
if(NOT lua_FOUND)
    message(WARNING "lua를 찾을 수 없습니다. sol2가 lua를 포함하고 있을 수 있습니다.")
    message(STATUS "vcpkg 설치: .\\vcpkg install lua:x64-mingw-static")
    message(STATUS "현재 CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
    # sol2가 있으면 lua 없이도 작동할 수 있으므로 경고만 표시하고 계속 진행
    if(sol2_FOUND)
        message(STATUS "sol2가 발견되었으므로 lua 없이 진행합니다.")
    else()
        message(FATAL_ERROR "lua 또는 sol2가 필요합니다.")
    endif()
endif()

add_executable(main
  main.cpp
  src/scene.cpp
  src/resource/resourceManager.cpp
  src/scripting/scriptManager.cpp
  src/scripting/luaScriptExecutor.cpp
  src/ui/uiManager.cpp
  src/widgets/UIWidget.cpp
  src/widgets/WidgetManager.cpp
  src/widgets/basic/ButtonWidget.cpp
  src/widgets/basic/TextWidget.cpp
  src/widgets/basic/BackgroundTextWidget.cpp
  src/widgets/basic/BackgroundWidget.cpp
  src/widgets/basic/EditTextWidget.cpp
  src/widgets/list/TextListWidget.cpp
  src/widgets/list/ChatListWidget.cpp
  src/widgets/list/MultiTypeListWidget.cpp
  src/widgets/list/UpgradeListWidget.cpp
  src/widgets/list/BannerListWidget.cpp
  src/widgets/list/SectionGridWidget.cpp
  src/widgets/list/VerticalGridWidget.cpp
  src/widgets/dialog/StandardDialogWidget.cpp
  src/widgets/dialog/CustomDialogWidget.cpp
  src/widgets/dialog/ToastWidget.cpp
  src/widgets/rpg/TilemapWidget.cpp
  src/animation/Animator.cpp
  src/animation/AnimationManager.cpp
  src/rendering/TextRenderer.cpp
  src/rendering/ImageRenderer.cpp
  src/utils/logger.cpp
  src/utils/FileIO.cpp
  src/data/userDataManager.cpp
  src/ads/StubAdProvider.cpp
)

# Windows에서 콘솔 창 활성화 (MinGW용)
if(WIN32 AND MINGW)
    set_target_properties(main PROPERTIES
        LINK_FLAGS "-Wl,-subsystem,console"
    )
elseif(WIN32)
    # MSVC용
    set_target_properties(main PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
endif()

if(WIN32)
    # Windows용 include 및 link 설정
    target_include_directories(main PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
        "${SDL2_IMAGE_ROOT}/include"
        "${SDL2_TTF_ROOT}/include"
    )
    
    # SDL2main은 SDL2보다 먼저 링크되어야 함
    target_link_libraries(main
        SDL2::SDL2main
        SDL2::SDL2
        SDL2_image::SDL2_image
        SDL2_ttf::SDL2_ttf
        JsonCpp::JsonCpp
        nlohmann_json::nlohmann_json
        sol2::sol2
    )
    
    # sol2는 lua를 필요로 하므로 lua 라이브러리를 링크해야 함
    # vcpkg에서 lua 라이브러리 파일 찾기
    if(DEFINED VCPKG_ROOT)
        set(VCPKG_INSTALLED_DIR "${VCPKG_ROOT}/installed/x64-mingw-static")
    elseif(DEFINED ENV{VCPKG_ROOT})
        set(VCPKG_INSTALLED_DIR "$ENV{VCPKG_ROOT}/installed/x64-mingw-static")
    endif()
    
    if(DEFINED VCPKG_INSTALLED_DIR AND EXISTS "${VCPKG_INSTALLED_DIR}/lib")
        file(GLOB LUA_LIBS "${VCPKG_INSTALLED_DIR}/lib/liblua*.a")
        if(LUA_LIBS)
            target_link_libraries(main ${LUA_LIBS})
            message(STATUS "Found lua libraries: ${LUA_LIBS}")
        else()
            # lua 라이브러리 이름이 다를 수 있으므로 직접 지정
            find_library(LUA_LIBRARY
                NAMES lua lua5.4 lua54
                PATHS "${VCPKG_INSTALLED_DIR}/lib"
                NO_DEFAULT_PATH
            )
            if(LUA_LIBRARY)
                target_link_libraries(main ${LUA_LIBRARY})
                message(STATUS "Found lua library: ${LUA_LIBRARY}")
            else()
                message(WARNING "Could not find lua library, sol2 may not work correctly")
            endif()
        endif()
    endif()
    
    # DLL 파일을 빌드 디렉토리로 복사
    add_custom_command(TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL2_DLL} $<TARGET_FILE_DIR:main>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL2_IMAGE_DLL} $<TARGET_FILE_DIR:main>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL2_TTF_DLL} $<TARGET_FILE_DIR:main>
    )
elseif(APPLE)
    # macOS용 include 및 link 설정
    target_include_directories(main PRIVATE
        ${CMAKE_SOURCE_DIR}
    )
    
    # C++ 표준 라이브러리 경로 명시적 추가
    # SDK 내부의 C++ 표준 라이브러리 경로 추가
    if(CMAKE_OSX_SYSROOT)
        target_include_directories(main SYSTEM PRIVATE
            ${CMAKE_OSX_SYSROOT}/usr/include/c++/v1
        )
    else()
        # 기본 경로 시도
        target_include_directories(main SYSTEM PRIVATE
            /Library/Developer/CommandLineTools/usr/include/c++/v1
        )
    endif()
    
    target_link_libraries(main
        SDL2::SDL2
        SDL2_image::SDL2_image
        SDL2_ttf::SDL2_ttf
        JsonCpp::JsonCpp
        nlohmann_json::nlohmann_json
        sol2::sol2
    )
    
    # lua가 있으면 링크, 없으면 sol2가 포함하고 있을 수 있음
    if(lua_FOUND)
        target_link_libraries(main lua)
    endif()
else()
    # Linux용 include 및 link 설정
    target_include_directories(main PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
    )
    
    target_link_libraries(main
        ${SDL2_LIBRARIES}
        ${SDL2_IMAGE_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
        JsonCpp::JsonCpp
        nlohmann_json::nlohmann_json
        sol2::sol2
    )
    
    # lua가 있으면 링크, 없으면 sol2가 포함하고 있을 수 있음
    if(lua_FOUND)
        target_link_libraries(main lua)
    endif()
endif()
