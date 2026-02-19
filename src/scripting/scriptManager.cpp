#include "scriptManager.h"
#include "../widgets/WidgetManager.h"
#include "../widgets/basic/ButtonWidget.h"
#include "../widgets/basic/TextWidget.h"
#include "../widgets/basic/BackgroundTextWidget.h"
#include "../widgets/basic/EditTextWidget.h"
#include "../widgets/list/TextListWidget.h"
#include "../widgets/list/ChatListWidget.h"
#include "../widgets/list/MultiTypeListWidget.h"
#include "../widgets/list/UpgradeListWidget.h"
#include "../widgets/list/BannerListWidget.h"
#include "../widgets/list/VerticalGridWidget.h"
#include "../widgets/list/SectionGridWidget.h"
#include "../widgets/dialog/StandardDialogWidget.h"
#include "../widgets/dialog/CustomDialogWidget.h"
#include "../widgets/dialog/ToastWidget.h"
#include "../animation/Animator.h"
#include "../data/userDataManager.h"
#include "../resource/resourceManager.h"
#include "../ads/StubAdProvider.h"
#include "../utils/logger.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <ctime>

ScriptManager::ScriptManager() {
    adProvider = std::make_unique<StubAdProvider>();
    setCommonApi();
    // 바인딩은 생성자에서 한 번만 등록 (씬 전환 시 재등록 불필요)
}

void ScriptManager::reset() {
    // Lua VM은 유지하고 스크립트 함수만 제거 (성능 최적화)
    auto& lua = executor.getState();
    
    // 씬별 스크립트 함수 제거
    lua["init"] = sol::lua_nil;
    lua["update"] = sol::lua_nil;
    lua["keyPressed"] = sol::lua_nil;
    
    // 씬별 전역 변수 제거 (선택적)
    // lua["SceneData"] = sol::nil;  // 필요시 추가
    
    // 바인딩은 유지 (재등록 불필요)
    // Lua VM 재생성 및 바인딩 재등록은 비효율적이므로 제거
}

void ScriptManager::loadScript(const std::string& code) {
    executor.loadScriptFromString(code);  // 문자열 Lua 코드 등록
}

void ScriptManager::setCommonApi() {
    auto& lua = executor.getState();
    sol::table uiTable = lua.create_table();

    // UI 함수들
    uiTable.set_function("debug", [](const std::string& name) {
        // Debug function (no-op)
    });

    uiTable.set_function("showScene", [this](const std::string& name) {
        this->loadScene(name);
    });

    lua["Scene"] = uiTable;

    // GlobalData: Lua에서 어디서든 접근 가능한 키-값 저장소
    sol::table globalTable = lua.create_table();
    globalTable.set_function("set", [this](const std::string& key, sol::object value) {
        nlohmann::json j = luaObjectToJson(value);
        globalData[key] = j.dump();
    });
    globalTable.set_function("get", [this](const std::string& key) -> sol::object {
        auto it = globalData.find(key);
        if (it == globalData.end() || it->second.empty()) {
            return sol::lua_nil;
        }
        try {
            nlohmann::json j = nlohmann::json::parse(it->second);
            auto& state = executor.getState();
            if (j.is_number_integer()) {
                return sol::make_object(state, j.get<int64_t>());
            } else if (j.is_number_float()) {
                return sol::make_object(state, j.get<double>());
            } else if (j.is_string()) {
                return sol::make_object(state, j.get<std::string>());
            } else if (j.is_boolean()) {
                return sol::make_object(state, j.get<bool>());
            } else {
                sol::table t = jsonToLuaTable(state, j);
                return sol::object(state, sol::make_reference(state, t));
            }
        } catch (...) {
            return sol::lua_nil;
        }
    });
    globalTable.set_function("remove", [this](const std::string& key) {
        globalData.erase(key);
    });
    lua["Global"] = globalTable;

    // require(modname): 리소스에 등록된 Lua 스크립트(텍스트)를 모듈로 로드. 모듈은 return M 형태로 반환.
    lua.set_function("require", [this](const std::string& modname) -> sol::object {
        if (!resourceManager) {
            Log::error("[Lua] require: ResourceManager not set");
            return sol::lua_nil;
        }
        auto& state = executor.getState();
        if (!state["package"].valid()) state["package"] = state.create_table();
        if (!state["package"]["loaded"].valid()) state["package"]["loaded"] = state.create_table();
        sol::table loaded = state["package"]["loaded"];
        if (loaded[modname].valid() && loaded[modname].get_type() != sol::type::lua_nil) {
            return loaded[modname];
        }
        std::string code = resourceManager->getText(modname);
        if (code.empty()) {
            Log::error("[Lua] require: module not found: ", modname);
            return sol::lua_nil;
        }
        std::string chunkname = "@" + modname;
        sol::load_result lr = state.load(code, chunkname);
        if (!lr.valid()) {
            sol::error err = lr;
            Log::error("[Lua] require: load error for ", modname, ": ", err.what());
            return sol::lua_nil;
        }
        sol::protected_function_result pfr = lr();
        if (!pfr.valid()) {
            sol::error err = pfr;
            Log::error("[Lua] require: run error for ", modname, ": ", err.what());
            return sol::lua_nil;
        }
        sol::object result = pfr.get<sol::object>(0);
        loaded[modname] = result;
        return result;
    });

    // Time API: 날짜/시각 및 경과 시간 (getMs 저장값과 diff* 동일 기준)
    sol::table timeTable = lua.create_table();
    timeTable.set_function("getDate", [this]() {
        auto& state = executor.getState();
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm* lt = std::localtime(&t);
        if (!lt) return sol::object(state, sol::lua_nil);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count() % 1000;
        sol::table tbl = state.create_table();
        tbl["year"]  = lt->tm_year + 1900;
        tbl["month"] = lt->tm_mon + 1;
        tbl["day"]   = lt->tm_mday;
        tbl["hour"]  = lt->tm_hour;
        tbl["min"]   = lt->tm_min;
        tbl["sec"]   = lt->tm_sec;
        tbl["ms"]    = static_cast<int>(ms);
        return sol::object(state, tbl);
    });
    timeTable.set_function("getMs", []() {
        auto now = std::chrono::system_clock::now();
        return static_cast<double>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()
            ).count()
        );
    });
    timeTable.set_function("diffSec", [](double saved_ms) {
        if (saved_ms <= 0) return 0.0;
        auto now = std::chrono::system_clock::now();
        double now_ms = static_cast<double>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()
            ).count()
        );
        double diff_ms = now_ms - saved_ms;
        return diff_ms < 0 ? 0.0 : (diff_ms / 1000.0);
    });
    timeTable.set_function("diffMin", [](double saved_ms) {
        if (saved_ms <= 0) return 0.0;
        auto now = std::chrono::system_clock::now();
        double now_ms = static_cast<double>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()
            ).count()
        );
        double diff_ms = now_ms - saved_ms;
        return diff_ms < 0 ? 0.0 : (diff_ms / 1000.0 / 60.0);
    });
    timeTable.set_function("diffHour", [](double saved_ms) {
        if (saved_ms <= 0) return 0.0;
        auto now = std::chrono::system_clock::now();
        double now_ms = static_cast<double>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()
            ).count()
        );
        double diff_ms = now_ms - saved_ms;
        return diff_ms < 0 ? 0.0 : (diff_ms / 1000.0 / 3600.0);
    });
    timeTable.set_function("diffDay", [](double saved_ms) {
        if (saved_ms <= 0) return 0.0;
        auto now = std::chrono::system_clock::now();
        double now_ms = static_cast<double>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()
            ).count()
        );
        double diff_ms = now_ms - saved_ms;
        return diff_ms < 0 ? 0.0 : (diff_ms / 1000.0 / 86400.0);
    });
    lua["Time"] = timeTable;

    // UserData API 바인딩
    setUserDataApi();
    
    // Resource API 바인딩
    setResourceApi();
}

void ScriptManager::setUiManager(UiManager* ui) {
    uiManager = ui;

    auto& lua = executor.getState();

    sol::table uiTable = lua.create_table();


    // UI 함수들
    uiTable.set_function("changeImage", [ui](const std::string& name, const std::string& image) {
        ui->changeTexture(name, image);
    });


    // Setter 함수들
    uiTable.set_function("move", [ui](const std::string& name, int x, int y) {
        ui->move(name, x, y);
    });

    uiTable.set_function("resize", [ui](const std::string& name, int w, int h) {
        ui->resize(name, w, h);
    });

    uiTable.set_function("setRect", [ui](const std::string& name, int x, int y, int w, int h) {
        ui->setRect(name, x, y, w, h);
    });

    uiTable.set_function("moveto", [ui](const std::string& name, int x, int y) {
        ui->moveTo(name, x, y);
    });

    uiTable.set_function("setAlpha", [ui](const std::string& name, float alpha) {
        ui->setAlpha(name, alpha);
    });
    uiTable.set_function("setVisible", [ui](const std::string& name, bool visible) {
        ui->setVisible(name, visible);
    });

    uiTable.set_function("setRotate", [ui](const std::string& name, float angle) {
        ui->setRotate(name, angle);
    });

    uiTable.set_function("setScale", [ui](const std::string& name, float scale) {
        ui->setScale(name, scale);
    });


    // Getter 함수들
    uiTable.set_function("getAlpha", [ui](const std::string& name) {
        return ui->getAlpha(name);
    });

    uiTable.set_function("getScale", [ui](const std::string& name) {
        return ui->getScale(name);
    });

    uiTable.set_function("getRotate", [ui](const std::string& name) {
        return ui->getRotate(name);
    });

    uiTable.set_function("getLeft", [ui](const std::string& name) {
        return ui->getLeft(name);
    });

    uiTable.set_function("getTop", [ui](const std::string& name) {
        return ui->getTop(name);
    });

    uiTable.set_function("getRight", [ui](const std::string& name) {
        return ui->getRight(name);
    });

    uiTable.set_function("getBottom", [ui](const std::string& name) {
        return ui->getBottom(name);
    });

    uiTable.set_function("getWidth", [ui](const std::string& name) {
        return ui->getWidth(name);
    });

    uiTable.set_function("getHeight", [ui](const std::string& name) {
        return ui->getHeight(name);
    });

    uiTable.set_function("isVisible", [ui](const std::string& name) {
        return ui->isVisible(name);
    });
    

    lua["ui"] = uiTable;  // 🔥 이제 Lua에서 ui.move(...) 사용 가능
}

void ScriptManager::setWidgetManager(WidgetManager* wm) {
    widgetManager = wm;
    if (!widgetManager) return;
    
    auto& lua = executor.getState();
    sol::table widgetTable = lua.create_table();
    
    // 버튼 관련
    widgetTable.set_function("setButtonCallback", [wm](const std::string& name, sol::function callback) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* button = dynamic_cast<ButtonWidget*>(widget)) {
                button->setOnClick([callback]() {
                    callback();
                });
            }
        }
    });
    
    widgetTable.set_function("setButtonEnabled", [wm](const std::string& name, bool enabled) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* button = dynamic_cast<ButtonWidget*>(widget)) {
                button->setEnabled(enabled);
            }
        }
    });
    
    widgetTable.set_function("setButtonNormalImage", [wm](const std::string& name, 
                                                          const std::string& imageName,
                                                          bool useNinePatch = false,
                                                          bool useThreePatch = false) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* button = dynamic_cast<ButtonWidget*>(widget)) {
                button->setNormalImage(imageName, useNinePatch, useThreePatch);
            }
        }
    });

    widgetTable.set_function("setButtonIcon", [wm](const std::string& name,
                                                   const std::string& iconImageName) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* button = dynamic_cast<ButtonWidget*>(widget)) {
                button->setIcon(iconImageName);
            }
        }
    });

    // 텍스트 위젯 관련 (TextWidget, ButtonWidget 지원)
    widgetTable.set_function("setText", [wm](const std::string& name, const std::string& text) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* textWidget = dynamic_cast<TextWidget*>(widget)) {
                textWidget->setText(text);
            } else if (auto* buttonWidget = dynamic_cast<ButtonWidget*>(widget)) {
                buttonWidget->setText(text);
            } else if (auto* bgTextWidget = dynamic_cast<BackgroundTextWidget*>(widget)) {
                bgTextWidget->setText(text);
            }
        }
    });
    
    widgetTable.set_function("getText", [wm](const std::string& name) -> std::string {
        if (!wm) return "";
        if (auto* widget = wm->getWidget(name)) {
            if (auto* textWidget = dynamic_cast<TextWidget*>(widget)) {
                return textWidget->getText();
            } else if (auto* bgTextWidget = dynamic_cast<BackgroundTextWidget*>(widget)) {
                return bgTextWidget->getText();
            }
        }
        return "";
    });
    widgetTable.set_function("getUIElementId", [wm](const std::string& name) -> std::string {
        if (!wm) return "";
        UIWidget* w = wm->getWidget(name);
        return w ? w->getUIElementIdentifier() : "";
    });
    
    // EditText 위젯 관련
    widgetTable.set_function("getEditText", [wm](const std::string& name) -> std::string {
        if (!wm) return "";
        if (auto* widget = wm->getWidget(name)) {
            if (auto* editTextWidget = dynamic_cast<EditTextWidget*>(widget)) {
                return editTextWidget->getText();
            }
        }
        return "";
    });
    
    widgetTable.set_function("setEditText", [wm](const std::string& name, const std::string& text) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* editTextWidget = dynamic_cast<EditTextWidget*>(widget)) {
                editTextWidget->setText(text);
            }
        }
    });
    
    // TextList 위젯 관련
    widgetTable.set_function("addListItem", [wm](const std::string& name, const std::string& item) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* textListWidget = dynamic_cast<TextListWidget*>(widget)) {
                textListWidget->addItem(item);
            }
        }
    });
    
    widgetTable.set_function("clearListItems", [wm](const std::string& name) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* textListWidget = dynamic_cast<TextListWidget*>(widget)) {
                textListWidget->clearItems();
            }
        }
    });
    
    // ChatList 위젯 관련
    widgetTable.set_function("addChatMessage", [wm](const std::string& name, const std::string& text, 
                                                    const std::string& iconName, const std::string& alignment) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* chatWidget = dynamic_cast<ChatListWidget*>(widget)) {
                ChatAlignment align = ChatAlignment::LEFT;
                if (alignment == "right" || alignment == "RIGHT") {
                    align = ChatAlignment::RIGHT;
                }
                chatWidget->addMessage(text, iconName, align);
                // 새 메시지 추가 후 자동으로 맨 아래로 스크롤
                chatWidget->scrollToBottom();
            }
        }
    });
    
    widgetTable.set_function("setChatListBackgroundImage", [wm](const std::string& name, 
                                                                 const std::string& imageName,
                                                                 bool useNinePatch = false,
                                                                 bool useThreePatch = false) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* chatWidget = dynamic_cast<ChatListWidget*>(widget)) {
                chatWidget->setBackgroundImage(imageName, useNinePatch, useThreePatch);
            }
        }
    });
    
    // MultiTypeList 위젯 관련
    widgetTable.set_function("addMultiTypeListItem", [wm](const std::string& name, 
                                                          const std::string& typeStr,
                                                          const std::string& text,
                                                          const std::string& iconName,
                                                          const std::string& imageName) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* multiTypeWidget = dynamic_cast<MultiTypeListWidget*>(widget)) {
                ListItemType itemType = ListItemType::CENTER_TEXT;
                if (typeStr == "center_text" || typeStr == "CENTER_TEXT") {
                    itemType = ListItemType::CENTER_TEXT;
                } else if (typeStr == "left_icon_text" || typeStr == "LEFT_ICON_TEXT") {
                    itemType = ListItemType::LEFT_ICON_TEXT;
                } else if (typeStr == "right_icon_text" || typeStr == "RIGHT_ICON_TEXT") {
                    itemType = ListItemType::RIGHT_ICON_TEXT;
                } else if (typeStr == "center_image" || typeStr == "CENTER_IMAGE") {
                    itemType = ListItemType::CENTER_IMAGE;
                }
                multiTypeWidget->addItem(itemType, text, iconName, imageName);
                multiTypeWidget->scrollToBottom();
            }
        }
    });
    
    widgetTable.set_function("setMultiTypeListBackgroundImage", [wm](const std::string& name, 
                                                                      const std::string& imageName,
                                                                      bool useNinePatch = false,
                                                                      bool useThreePatch = false) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* multiTypeWidget = dynamic_cast<MultiTypeListWidget*>(widget)) {
                multiTypeWidget->setBackgroundImage(imageName, useNinePatch, useThreePatch);
            }
        }
    });
    
    // UpgradeList 위젯 관련
    widgetTable.set_function("addUpgradeListItem", [wm](const std::string& name,
                                                        const std::string& iconName,
                                                        const std::string& titleText,
                                                        const std::string& descText,
                                                        const std::string& buttonImage,
                                                        const std::string& buttonText) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* upgradeWidget = dynamic_cast<UpgradeListWidget*>(widget)) {
                upgradeWidget->addItem(iconName, titleText, descText, buttonImage, buttonText);
            }
        }
    });
    widgetTable.set_function("clearUpgradeListItems", [wm](const std::string& name) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* upgradeWidget = dynamic_cast<UpgradeListWidget*>(widget)) {
                upgradeWidget->clearItems();
            }
        }
    });
    widgetTable.set_function("setUpgradeListButtonCallback", [wm](const std::string& name,
                                                                  sol::function callback) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* upgradeWidget = dynamic_cast<UpgradeListWidget*>(widget)) {
                upgradeWidget->setOnItemButtonClick([callback](int index) {
                    callback(index);
                });
            }
        }
    });
    widgetTable.set_function("setUpgradeListItemBackground", [wm](const std::string& name,
                                                                  const std::string& imageName,
                                                                  bool useNinePatch = false,
                                                                  bool useThreePatch = false) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* upgradeWidget = dynamic_cast<UpgradeListWidget*>(widget)) {
                upgradeWidget->setItemBackgroundImage(imageName, useNinePatch, useThreePatch);
            }
        }
    });

    // BannerList 위젯 관련
    widgetTable.set_function("addBannerListItem", [wm](const std::string& name,
                                                       const std::string& title,
                                                       sol::optional<std::string> imageName) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* bannerWidget = dynamic_cast<BannerListWidget*>(widget)) {
                if (imageName) {
                    bannerWidget->addItem(title, *imageName);
                } else {
                    bannerWidget->addItem(title);
                }
            }
        }
    });
    widgetTable.set_function("clearBannerListItems", [wm](const std::string& name) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* bannerWidget = dynamic_cast<BannerListWidget*>(widget)) {
                bannerWidget->clearItems();
            }
        }
    });
    widgetTable.set_function("setBannerListClickCallback", [wm](const std::string& name,
                                                                sol::function callback) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* bannerWidget = dynamic_cast<BannerListWidget*>(widget)) {
                bannerWidget->setOnItemClick([callback](int index) {
                    callback(index);
                });
            }
        }
    });
    
    // VerticalGrid 위젯 관련
    widgetTable.set_function("addGridItem", [wm](const std::string& name, const std::string& imageName) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* gridWidget = dynamic_cast<VerticalGridWidget*>(widget)) {
                gridWidget->addItem(imageName);
            }
        }
    });
    widgetTable.set_function("clearGridItems", [wm](const std::string& name) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* gridWidget = dynamic_cast<VerticalGridWidget*>(widget)) {
                gridWidget->clearItems();
            }
        }
    });
    // SectionGrid 위젯 관련
    widgetTable.set_function("addSectionOverlay", [wm](const std::string& name,
                                                       const std::string& overlayId,
                                                       const std::string& imageName,
                                                       sol::optional<int> height) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* sectionWidget = dynamic_cast<SectionGridWidget*>(widget)) {
                sectionWidget->addOverlay(overlayId, imageName, height.value_or(36));
            }
        }
    });
    widgetTable.set_function("addSectionHeader", [wm](const std::string& name,
                                                      const std::string& imageName) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* sectionWidget = dynamic_cast<SectionGridWidget*>(widget)) {
                sectionWidget->addHeader(imageName);
            }
        }
    });
    widgetTable.set_function("addSectionCard", [wm](const std::string& name,
                                                     const std::string& imageName,
                                                     sol::optional<sol::object> overlayArg) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* sectionWidget = dynamic_cast<SectionGridWidget*>(widget)) {
                std::string overlayId;
                if (overlayArg) {
                    sol::object o = *overlayArg;
                    if (o.is<bool>()) {
                        overlayId = o.as<bool>() ? "locked" : "";
                    } else if (o.is<std::string>()) {
                        overlayId = o.as<std::string>();
                    }
                }
                sectionWidget->addCard(imageName, overlayId);
            }
        }
    });
    widgetTable.set_function("clearSectionGrid", [wm](const std::string& name) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* sectionWidget = dynamic_cast<SectionGridWidget*>(widget)) {
                sectionWidget->clear();
            }
        }
    });
    widgetTable.set_function("setSectionGridHeaderScale", [wm](const std::string& name, double scale) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* sectionWidget = dynamic_cast<SectionGridWidget*>(widget)) {
                sectionWidget->setHeaderImageScale(static_cast<float>(scale));
            }
        }
    });
    widgetTable.set_function("layoutSectionGrid", [wm](const std::string& name) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* sectionWidget = dynamic_cast<SectionGridWidget*>(widget)) {
                sectionWidget->layout();
            }
        }
    });
    
    // StandardDialog 위젯 관련
    widgetTable.set_function("setDialogContent", [wm](const std::string& name, 
                                                      const std::string& iconName,
                                                      const std::string& title,
                                                      const std::string& description) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* dialogWidget = dynamic_cast<StandardDialogWidget*>(widget)) {
                dialogWidget->setContent(iconName, title, description);
            }
        }
    });
    
    widgetTable.set_function("showDialog", [wm](const std::string& name) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* dialogWidget = dynamic_cast<StandardDialogWidget*>(widget)) {
                dialogWidget->show();
            }
        }
    });
    
    widgetTable.set_function("hideDialog", [wm](const std::string& name) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* dialogWidget = dynamic_cast<StandardDialogWidget*>(widget)) {
                dialogWidget->hide();
            }
        }
    });
    
    widgetTable.set_function("setDialogOnOk", [wm](const std::string& name, sol::function callback) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* dialogWidget = dynamic_cast<StandardDialogWidget*>(widget)) {
                dialogWidget->setOnOk([callback]() {
                    callback();
                });
            }
        }
    });
    
    widgetTable.set_function("setDialogOnCancel", [wm](const std::string& name, sol::function callback) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* dialogWidget = dynamic_cast<StandardDialogWidget*>(widget)) {
                dialogWidget->setOnCancel([callback]() {
                    callback();
                });
            }
        }
    });
    
    // CustomDialog 위젯 관련 (제어만 가능, 생성 불가)
    widgetTable.set_function("showCustomDialog", [wm](const std::string& name) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* customDialogWidget = dynamic_cast<CustomDialogWidget*>(widget)) {
                customDialogWidget->show();
            }
        }
    });
    
    widgetTable.set_function("hideCustomDialog", [wm](const std::string& name) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* customDialogWidget = dynamic_cast<CustomDialogWidget*>(widget)) {
                customDialogWidget->hide();
            }
        }
    });
    
    widgetTable.set_function("dismissCustomDialog", [wm](const std::string& name) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* customDialogWidget = dynamic_cast<CustomDialogWidget*>(widget)) {
                customDialogWidget->dismiss();
            }
        }
    });
    
    // Toast 위젯 관련
    widgetTable.set_function("showToast", [wm](const std::string& name, const std::string& text, int durationMs) {
        if (!wm) {
            std::cerr << "[Lua] showToast: WidgetManager is null" << std::endl;
            return;
        }
        auto* widget = wm->getWidget(name);
        if (!widget) {
            std::cerr << "[Lua] showToast: Widget not found: " << name << std::endl;
            return;
        }
        if (auto* toastWidget = dynamic_cast<ToastWidget*>(widget)) {
            toastWidget->showToast(text, durationMs);
        } else {
            std::cerr << "[Lua] showToast: Widget is not a ToastWidget: " << name << std::endl;
        }
    });
    
    widgetTable.set_function("hideToast", [wm](const std::string& name) {
        if (!wm) return;
        if (auto* widget = wm->getWidget(name)) {
            if (auto* toastWidget = dynamic_cast<ToastWidget*>(widget)) {
                toastWidget->hide();
            }
        }
    });
    
    // 위젯 공통 메서드 (필요시 확장)
    widgetTable.set_function("getWidget", [wm](const std::string& name) {
        if (!wm) return false;
        return wm->getWidget(name) != nullptr;
    });
    
    lua["widget"] = widgetTable;  // Lua에서 widget.setButtonCallback(...) 사용
}

class AnimatorWrapper {
    public:
        std::shared_ptr<Animator> impl;
    
        AnimatorWrapper(const std::string& name) {
            impl = std::make_shared<Animator>(name);
        }
    
        AnimatorWrapper* rotate(float endDeg, int dur) {
            impl->rotate(endDeg, dur);
            return this;
        }
    
        AnimatorWrapper* rotate(float startDeg, float endDeg, int dur) {
            impl->rotate(startDeg, endDeg, dur);
            return this;
        }
    
        AnimatorWrapper* move(int dx, int dy, int dur) {
            impl->move(dx, dy, dur);
            return this;
        }
    
        AnimatorWrapper* moveTo(int x, int y, int dur) {
            impl->moveTo(x, y, dur);
            return this;
        }
    
        AnimatorWrapper* moveTo(int sx, int sy, int ex, int ey, int dur) {
            impl->moveTo(sx, sy, ex, ey, dur);
            return this;
        }
    
        AnimatorWrapper* resize(int w, int h, int dur) {
            impl->resize(w, h, dur);
            return this;
        }
    
        AnimatorWrapper* resize(int sw, int sh, int ew, int eh, int dur) {
            impl->resize(sw, sh, ew, eh, dur);
            return this;
        }
    
        AnimatorWrapper* alpha(float endAlpha, int dur) {
            impl->alpha(endAlpha, dur);
            return this;
        }
    
        AnimatorWrapper* alpha(float startAlpha, float endAlpha, int dur) {
            impl->alpha(startAlpha, endAlpha, dur);
            return this;
        }
    
        AnimatorWrapper* scale(float endScale, int dur) {
            impl->scale(endScale, dur);
            return this;
        }
    
        AnimatorWrapper* scale(float startScale, float endScale, int dur) {
            impl->scale(startScale, endScale, dur);
            return this;
        }
    
        AnimatorWrapper* changeTexture(const std::string& imageName) {
            impl->changeTexture(imageName);
            return this;
        }

        AnimatorWrapper* setVisible(bool visible) {
            impl->setVisible(visible);
            return this;
        }

        AnimatorWrapper* delay(int dur) {
            impl->delay(dur);
            return this;
        }
    
        AnimatorWrapper* repeat_anim(int count) {
            impl->repeat(count);
            return this;
        }
    
        AnimatorWrapper* callback(sol::function func) {
            impl->callback([func]() {
                func();
            });
            return this;
        }
    
        std::shared_ptr<Animator> get() const {
            return impl;
        }
    };

void ScriptManager::setAnimationManager(AnimationManager* am) {
    animationManager = am;
    auto& lua = executor.getState();

    // Animator 클래스 바인딩
    lua.new_usertype<AnimatorWrapper>("Animator",
        "rotate", sol::overload(
            static_cast<AnimatorWrapper* (AnimatorWrapper::*)(float, int)>(&AnimatorWrapper::rotate),
            static_cast<AnimatorWrapper* (AnimatorWrapper::*)(float, float, int)>(&AnimatorWrapper::rotate)
        ),
        "move", &AnimatorWrapper::move,
        "moveTo", sol::overload(
            static_cast<AnimatorWrapper* (AnimatorWrapper::*)(int, int, int)>(&AnimatorWrapper::moveTo),
            static_cast<AnimatorWrapper* (AnimatorWrapper::*)(int, int, int, int, int)>(&AnimatorWrapper::moveTo)
        ),
        "resize", sol::overload(
            static_cast<AnimatorWrapper* (AnimatorWrapper::*)(int, int, int)>(&AnimatorWrapper::resize),
            static_cast<AnimatorWrapper* (AnimatorWrapper::*)(int, int, int, int, int)>(&AnimatorWrapper::resize)
        ),
        "alpha", sol::overload(
            static_cast<AnimatorWrapper* (AnimatorWrapper::*)(float, int)>(&AnimatorWrapper::alpha),
            static_cast<AnimatorWrapper* (AnimatorWrapper::*)(float, float, int)>(&AnimatorWrapper::alpha)
        ),
        "scale", sol::overload(
            static_cast<AnimatorWrapper* (AnimatorWrapper::*)(float, int)>(&AnimatorWrapper::scale),
            static_cast<AnimatorWrapper* (AnimatorWrapper::*)(float, float, int)>(&AnimatorWrapper::scale)
        ),
        "changeTexture", &AnimatorWrapper::changeTexture,
        "setVisible", &AnimatorWrapper::setVisible,
        "delay", &AnimatorWrapper::delay,
        "repeat_anim", &AnimatorWrapper::repeat_anim,
        "callback", &AnimatorWrapper::callback,
        "get", &AnimatorWrapper::get
    );

    lua.set_function("Animator", [](const std::string& name) {
        return AnimatorWrapper(name);
    });

    // ✅ animation.add(uiElementName, shared_ptr<Animator>)
    // sol2는 shared_ptr을 자동으로 처리할 수 있으므로 별도 등록 불필요
    lua["animation"] = lua.create_table();
    lua["animation"]["add"] = [this](const std::string& uiElementName, std::shared_ptr<Animator> anim) {
        // 에러 체크: UI 요소 이름이 비어있는지 확인
        if (uiElementName.empty()) {
            std::cerr << "[ERROR] animation.add(): UI element name is empty!" << std::endl;
            return;
        }
        
        // 에러 체크: Animator가 null인지 확인
        if (!anim) {
            std::cerr << "[ERROR] animation.add(): Animator is null for UI element '" << uiElementName << "'!" << std::endl;
            return;
        }
        
        // 에러 체크: AnimationManager가 설정되어 있는지 확인
        if (!animationManager) {
            std::cerr << "[ERROR] animation.add(): AnimationManager is not set!" << std::endl;
            return;
        }
        
        animationManager->add(uiElementName, anim);
    };
    
    // JSON에서 애니메이션 로드
    lua["animation"]["loadFromJson"] = [this](const std::string& uiElementName, const std::string& animJsonName) {
        if (!animationManager) {
            std::cerr << "[ERROR] animation.loadFromJson(): AnimationManager is not set!" << std::endl;
            return;
        }
        
        if (uiElementName.empty()) {
            std::cerr << "[ERROR] animation.loadFromJson(): UI element name is empty!" << std::endl;
            return;
        }
        
        // AnimationManager에 위임
        if (!animationManager->loadAnimatorFromJson(uiElementName, animJsonName)) {
            std::cerr << "[ERROR] animation.loadFromJson(): Failed to load animation from JSON: " << animJsonName << std::endl;
        }
    };

    // Ads API (배너 on/off, 전면, 리워드 비디오 - 리워드는 콜백으로 성공/실패 전달)
    sol::table adsTable = lua.create_table();
    adsTable.set_function("setBannerVisible", [this](bool visible) {
        if (adProvider) adProvider->setBannerVisible(visible);
    });
    adsTable.set_function("showInterstitial", [this](sol::optional<sol::function> onClosed) {
        if (!adProvider) return;
        if (onClosed && onClosed.value().valid()) {
            adProvider->showInterstitial([this, onClosed]() {
                onClosed.value()();
            });
        } else {
            adProvider->showInterstitial(nullptr);
        }
    });
    adsTable.set_function("showRewardedVideo", [this](sol::function callback) {
        if (!adProvider) return;
        adProvider->showRewardedVideo([this, callback](bool success) {
            callback(success);
        });
    });
    lua["Ads"] = adsTable;
}

void ScriptManager::setLoadScene(LoadSceneFn fn) {
    loadSceneFn = std::move(fn);
}

void ScriptManager::loadScene(const std::string name) {
    if (loadSceneFn) {
        loadSceneFn(name);
    } else {
        Log::error("LoadScene function not set!");
    }
}

nlohmann::json ScriptManager::luaObjectToJson(sol::object obj) {
    if (obj.get_type() == sol::type::lua_nil) {
        return nlohmann::json();
    }
    if (obj.get_type() == sol::type::number) {
        double val = obj.as<double>();
        if (val == static_cast<int64_t>(val)) {
            return static_cast<int64_t>(val);
        }
        return val;
    } else if (obj.get_type() == sol::type::string) {
        return obj.as<std::string>();
    } else if (obj.get_type() == sol::type::boolean) {
        return obj.as<bool>();
    } else if (obj.get_type() == sol::type::table) {
        sol::table tbl = obj.as<sol::table>();
        nlohmann::json result;
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
            result = nlohmann::json::array();
            for (int i = 1; i <= maxIndex; ++i) {
                sol::object val = tbl[i];
                result.push_back(luaObjectToJson(val));
            }
        } else {
            result = nlohmann::json::object();
            for (auto pair : tbl) {
                std::string key;
                if (pair.first.get_type() == sol::type::string) {
                    key = pair.first.as<std::string>();
                } else {
                    key = std::to_string(pair.first.as<int>());
                }
                result[key] = luaObjectToJson(pair.second);
            }
        }
        return result;
    }
    return nlohmann::json();
}

sol::table ScriptManager::jsonToLuaTable(sol::state_view& lua, const nlohmann::json& json) {
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
                result[it.key()] = jsonToLuaTable(lua, value);
            }
        }
    } else if (json.is_array()) {
        for (size_t i = 0; i < json.size(); ++i) {
            const auto& value = json[i];
            if (value.is_number_integer()) {
                result[i + 1] = value.get<int64_t>();
            } else if (value.is_number_float()) {
                result[i + 1] = value.get<double>();
            } else if (value.is_string()) {
                result[i + 1] = value.get<std::string>();
            } else if (value.is_boolean()) {
                result[i + 1] = value.get<bool>();
            } else if (value.is_array() || value.is_object()) {
                result[i + 1] = jsonToLuaTable(lua, value);
            }
        }
    }
    return result;
}

void ScriptManager::handleEvent(const SDL_Event& event, Uint32 mouseClickEvent) {
    // Scene에서 이미 필터링된 이벤트만 전달되므로 여기서는 변환과 Lua 호출만 수행
    
    // 좌표 및 키코드 추출
    int x = 0, y = 0, keycode = 0;
    std::string typeStr = "";
    
    if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
        x = event.button.x;
        y = event.button.y;
        typeStr = (event.type == SDL_MOUSEBUTTONDOWN) ? "mouse_down" : "mouse_up";
    } else if (event.type == SDL_MOUSEMOTION) {
        x = event.motion.x;
        y = event.motion.y;
        typeStr = "mouse_motion";
    } else if (event.type == mouseClickEvent) {
        int* coords = static_cast<int*>(event.user.data1);
        if (coords) {
            x = coords[0];
            y = coords[1];
        }
        typeStr = "mouse_click";
        keycode = event.user.code;
    } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        keycode = event.key.keysym.sym;
        typeStr = (event.type == SDL_KEYDOWN) ? "key_down" : "key_up";
    }
    
    // UIElement 찾기
    const UIElement* elem = nullptr;
    if (uiManager) {
        elem = uiManager->findElementByPosition(x, y);
    }
    std::string clickedElementName = "";
    if (elem != nullptr) clickedElementName = elem->name;

    // 마우스/키 이벤트만 전달
    call("keyPressed", clickedElementName, typeStr, keycode, x, y);
    
    // SDL_USER 이벤트의 메모리 해제
    if (event.type == mouseClickEvent && event.user.data1) {
        delete[] static_cast<int*>(event.user.data1);
    }
}

void ScriptManager::setUserDataManager(UserDataManager* udm) {
    userDataManager = udm;
    setUserDataApi();
    setResourceApi();
}

void ScriptManager::setResourceManager(ResourceManager* rm) {
    resourceManager = rm;
}

void ScriptManager::setUserDataApi() {
    if (!userDataManager) return;
    
    auto& lua = executor.getState();
    sol::table userDataTable = lua.create_table();
    
    userDataTable.set_function("load", [this](const std::string& schemaName) {
        auto& lua = executor.getState();
        sol::table result = userDataManager->load(schemaName, lua);
        
        // save 메서드를 수정하여 Lua 테이블에서 다시 읽어서 저장하도록 함
        UserDataManager* manager = userDataManager;
        std::string name = schemaName;
        result["save"] = [manager, name, result]() {
            manager->saveFromLuaTable(name, result);
        };
        
        return result;
    });
    
    lua["UserData"] = userDataTable;
}

void ScriptManager::setResourceApi() {
    if (!userDataManager) return;
    
    auto& lua = executor.getState();
    sol::table resourceTable = lua.create_table();
    
    // JSON 파일을 Lua 테이블로 변환하는 함수
    resourceTable.set_function("getDataTable", [this](const std::string& jsonName) {
        auto& lua = executor.getState();
        return userDataManager->getJsonAsLuaTable(lua, jsonName);
    });
    
    lua["Resource"] = resourceTable;
}