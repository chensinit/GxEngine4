#include "WidgetManager.h"
#include "basic/ButtonWidget.h"
#include "basic/TextWidget.h"
#include "basic/BackgroundTextWidget.h"
#include "basic/BackgroundWidget.h"
#include "basic/EditTextWidget.h"
#include "list/TextListWidget.h"
#include "list/ChatListWidget.h"
#include "list/MultiTypeListWidget.h"
#include "list/UpgradeListWidget.h"
#include "list/BannerListWidget.h"
#include "list/VerticalGridWidget.h"
#include "list/SectionGridWidget.h"
#include "dialog/StandardDialogWidget.h"
#include "dialog/CustomDialogWidget.h"
#include "dialog/ToastWidget.h"
#include "rpg/TilemapWidget.h"
#include "../ui/uiManager.h"
#include "../rendering/TextRenderer.h"
#include "../rendering/ImageRenderer.h"
#include "../animation/AnimationManager.h"
#include "../utils/logger.h"

WidgetManager::WidgetManager(UiManager* uiMgr, ResourceManager* resMgr,
                             SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                             AnimationManager* animMgr)
    : uiManager(uiMgr), resourceManager(resMgr), 
      renderer(sdlRenderer), textRenderer(txtRenderer),
      animationManager(animMgr),
      focusedWidget(nullptr) {
}

void WidgetManager::addWidget(const std::string& name, std::unique_ptr<UIWidget> widget) {
    if (!widget) return;
    widgets[name] = std::move(widget);
}

UIWidget* WidgetManager::getWidget(const std::string& name) {
    auto it = widgets.find(name);
    if (it != widgets.end()) {
        return it->second.get();
    }
    return nullptr;
}

void WidgetManager::removeWidget(const std::string& name) {
    widgets.erase(name);
}

void WidgetManager::clear() {
    clearFocus();  // 포커스 해제
    widgets.clear();
}

void WidgetManager::update(float deltaTime) {
    for (auto& [name, widget] : widgets) {
        widget->update(deltaTime);
    }
}

void WidgetManager::handleEvents(const std::vector<SDL_Event>& events) {
    for (const auto& event : events) {
        // 1. 마우스 클릭: 포커스 관리만 (위젯별 처리는 handleEvent로 위임)
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            int x = event.button.x;
            int y = event.button.y;
            
            // 포커스 가능한 위젯 찾기 (EditText 등)
            UIWidget* focusableWidget = findWidgetAtPosition(x, y, true);
            if (focusableWidget != focusedWidget) {
                // 기존 포커스 해제
                if (focusedWidget) {
                    focusedWidget->setFocus(false);
                }
                
                // 새 포커스 설정
                focusedWidget = focusableWidget;
                if (focusedWidget && focusableWidget->canReceiveFocus()) {
                    focusableWidget->setFocus(true);
                } else {
                    focusedWidget = nullptr;
                }
            }
        }
        
        // 2. 포커스가 있는 위젯에 입력 이벤트 전달 (텍스트 입력 등)
        if (focusedWidget) {
            focusedWidget->handleEvent(event);
        }
        
        // 3. 모든 위젯에 이벤트 전달 (각 위젯이 자신의 영역인지 확인)
        // SDL_TEXTINPUT은 포커스된 위젯만 처리 (이미 위에서 처리됨)
        if (event.type != SDL_TEXTINPUT) {
            for (auto& [name, widget] : widgets) {
                // 포커스된 위젯은 이미 처리했으므로 제외
                if (widget.get() != focusedWidget) {
                    widget->handleEvent(event);
                }
            }
        }
    }
}

UIWidget* WidgetManager::findWidgetAtPosition(int x, int y, bool focusableOnly) {
    // 역순으로 검색 (나중에 추가된 위젯이 위에 있음)
    // unordered_map은 역순 반복자가 없으므로, 모든 위젯을 검색하고 마지막 매칭된 것을 반환
    UIWidget* found = nullptr;
    for (auto& [name, widget] : widgets) {
        if (widget->isPointInside(x, y)) {
            if (focusableOnly) {
                if (widget->canReceiveFocus()) {
                    found = widget.get();
                }
            } else {
                // 포커스 가능하거나 클릭 가능한 위젯 찾기
                if (widget->canReceiveFocus() || widget->canReceiveClick()) {
                    found = widget.get();
                }
            }
        }
    }
    return found;
}

void WidgetManager::applyElementProperties(struct UIElement* element, const nlohmann::json& uiElement) {
    if (!element) return;
    
    if (uiElement.contains("scale")) {
        element->scale = uiElement["scale"].get<float>();
    }
    if (uiElement.contains("rotate")) {
        element->rotation = uiElement["rotate"].get<float>();
    }
    if (uiElement.contains("alpha")) {
        element->alpha = uiElement["alpha"].get<float>();
    }
    if (uiElement.contains("visible")) {
        element->visible = uiElement["visible"].get<bool>();
    }
    if (uiElement.contains("clickable")) {
        element->clickable = uiElement["clickable"].get<bool>();
    }
}

void WidgetManager::applyParentRelationship(const std::string& elementId, const nlohmann::json& uiElement) {
    if (uiElement.contains("parent")) {
        std::string parentName = uiElement["parent"].get<std::string>();
        uiManager->setParent(elementId, parentName);
    }
}

void WidgetManager::setFocusedWidget(UIWidget* widget) {
    if (focusedWidget != widget) {
        if (focusedWidget) {
            focusedWidget->setFocus(false);
        }
        focusedWidget = widget;
        if (focusedWidget) {
            focusedWidget->setFocus(true);
        }
    }
}

void WidgetManager::clearFocus() {
    if (focusedWidget) {
        focusedWidget->setFocus(false);
        focusedWidget = nullptr;
    }
}

void WidgetManager::render(SDL_Renderer* renderer) {
    // 대부분의 위젯은 UIElement로 렌더링되므로 여기서는 특별한 렌더링만 처리
    // TilemapWidget, TextListWidget 같은 직접 렌더링 위젯만 처리
    // EditTextWidget, TextWidget, ButtonWidget은 UIElement로 렌더링되므로 UiManager::render()에서 처리됨
    for (auto& [name, widget] : widgets) {
        // EditTextWidget, TextWidget은 UIElement로 렌더링되므로 건너뛰기
        if (dynamic_cast<EditTextWidget*>(widget.get()) || dynamic_cast<TextWidget*>(widget.get())) {
            continue;
        }
        
        // 위젯의 UIElement ID 가져오기
        std::string elementId = widget->getUIElementIdentifier();
        if (elementId.empty()) continue;
        
        // 위젯의 UIElement 가져오기
        auto* element = uiManager->findElementByName(elementId);
        if (!element || !element->visible) continue;
        
        // 위젯의 월드 위치 계산 (부모 위치 포함)
        int widgetX, widgetY;
        uiManager->getWorldPosition(elementId, widgetX, widgetY);
        
        // 위젯 영역으로 클리핑 설정
        SDL_Rect clipRect = {
            widgetX,
            widgetY,
            static_cast<int>(element->rect.w * element->scale),
            static_cast<int>(element->rect.h * element->scale)
        };
        SDL_RenderSetClipRect(renderer, &clipRect);
        
        // TilemapWidget, TextListWidget 같은 직접 렌더링 위젯은 여기서 렌더링
        // (UIElement로 렌더링되는 위젯은 render()가 빈 구현이므로 문제없음)
        widget->render(renderer);
        
        // 클리핑 해제
        SDL_RenderSetClipRect(renderer, nullptr);
    }
}

bool WidgetManager::loadWidgetFromJson(const nlohmann::json& uiElement) {
    std::string type = uiElement["type"].get<std::string>();
    std::string uiName = uiElement["name"].get<std::string>();
    
    std::vector<int> loc;
    if (uiElement.contains("loc")) {
        loc = uiElement["loc"].get<std::vector<int>>();
    } else {
        loc = {0, 0, 0, 0};
    }
    
    if (type == "button") {
        std::string normalImg = uiElement["normalImage"].get<std::string>();
        std::string pressedImg = "";
        if (uiElement.contains("pressedImage")) {
            pressedImg = uiElement["pressedImage"].get<std::string>();
        }
        
        // ButtonWidget 생성 (rect 포함)
        SDL_Rect buttonRect = SDL_Rect{loc[0], loc[1], loc[2] - loc[0], loc[3] - loc[1]};
        auto button = std::make_unique<ButtonWidget>(
            uiManager, resourceManager, renderer, textRenderer,
            normalImg, pressedImg, buttonRect, animationManager
        );
        
        // 버튼의 UIElement ID 가져오기
        std::string buttonElementId = button->getUIElementIdentifier();
        
        // 버튼의 UIElement 속성 설정 (위젯 내부에서 관리하지만, JSON에서 지정된 속성은 여기서 설정)
        auto* element = uiManager->findElementByName(buttonElementId);
        if (element) {
            applyElementProperties(element, uiElement);
        }
        
        // 부모-자식 관계 설정
        applyParentRelationship(buttonElementId, uiElement);
        
        // 버튼 텍스트 설정 (JSON에서 읽기)
        if (uiElement.contains("text")) {
            std::string buttonText = uiElement["text"].get<std::string>();
            button->setText(buttonText);
        }
        
        // 텍스트 스타일 설정
        if (uiElement.contains("textSize")) {
            int textSize = uiElement["textSize"].get<int>();
            button->setFontSize(textSize);
        }
        
        if (uiElement.contains("textColor")) {
            auto rgb = uiElement["textColor"].get<std::vector<int>>();
            SDL_Color textColor = {255, 255, 255, 255};
            if (rgb.size() >= 3) {
                textColor.r = static_cast<Uint8>(rgb[0]);
                textColor.g = static_cast<Uint8>(rgb[1]);
                textColor.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) {
                    textColor.a = static_cast<Uint8>(rgb[3]);
                }
            }
            button->setTextColor(textColor);
        }
        
        // 9패치 / 3패치 설정 (JSON에서 읽기)
        bool useNinePatch = false;
        bool useThreePatch = false;
        if (uiElement.contains("useNinePatch")) {
            useNinePatch = uiElement["useNinePatch"].get<bool>();
        }
        if (uiElement.contains("useThreePatch")) {
            useThreePatch = uiElement["useThreePatch"].get<bool>();
        }
        if (useNinePatch || useThreePatch) {
            button->setNormalImage(normalImg, useNinePatch, useThreePatch);
        }
        
        addWidget(uiName, std::move(button));
        return true;
    }
    else if (type == "text") {
        std::string text = uiElement["text"].get<std::string>();
        int size = 12;
        if (uiElement.contains("textSize")) {
            size = uiElement["textSize"].get<int>();
        }
        
        SDL_Color color = {255, 255, 255, 255};
        if (uiElement.contains("textColor")) {
            auto rgb = uiElement["textColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                color.r = static_cast<Uint8>(rgb[0]);
                color.g = static_cast<Uint8>(rgb[1]);
                color.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) {
                    color.a = static_cast<Uint8>(rgb[3]);
                }
            }
        }
        
        int wrapWidth = loc[2] - loc[0];  // 기본 wrap width는 rect width
        int maxHeight = loc[3] - loc[1];  // 기본 max height는 rect height
        
        std::string textAlign;
        if (uiElement.contains("textAlign")) {
            textAlign = uiElement["textAlign"].get<std::string>();
        }
        int contW = loc[2] - loc[0];
        int contH = loc[3] - loc[1];
        
        // TextWidget 생성 (textAlign/container 전달 시 가운데 정렬 등 적용)
        auto textWidget = std::make_unique<TextWidget>(
            uiManager, resourceManager, renderer, textRenderer,
            text, size, color, wrapWidth, maxHeight,
            textAlign, loc[0], loc[1], contW, contH
        );
        
        // UIElement의 위치 및 속성 설정. textAlign "center"면 TextWidget이 updateTexture()에서 이미 설정함
        std::string elementId = textWidget->getUIElementId();
        auto* element = uiManager->findElementByName(elementId);
        if (element) {
            if (textAlign != "center") {
                element->rect.x = loc[0];
                element->rect.y = loc[1];
            }
            // width/height는 TextWidget이 텍스처 크기로 설정
            
            // 선택적 필드 처리
            applyElementProperties(element, uiElement);
            
            // 부모-자식 관계 설정
            applyParentRelationship(elementId, uiElement);
        }
        
        addWidget(uiName, std::move(textWidget));
        return true;
    }
    else if (type == "backgroundtext") {
        std::string text = uiElement["text"].get<std::string>();
        int size = 12;
        if (uiElement.contains("textSize")) {
            size = uiElement["textSize"].get<int>();
        }
        
        SDL_Color textColor = {0, 0, 0, 255};  // 검정색 기본값
        if (uiElement.contains("textColor")) {
            auto rgb = uiElement["textColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                textColor.r = static_cast<Uint8>(rgb[0]);
                textColor.g = static_cast<Uint8>(rgb[1]);
                textColor.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) {
                    textColor.a = static_cast<Uint8>(rgb[3]);
                }
            }
        }
        
        SDL_Color bgColor = {255, 255, 0, 255};  // 노란색 기본값
        if (uiElement.contains("backgroundColor")) {
            auto rgb = uiElement["backgroundColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                bgColor.r = static_cast<Uint8>(rgb[0]);
                bgColor.g = static_cast<Uint8>(rgb[1]);
                bgColor.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) {
                    bgColor.a = static_cast<Uint8>(rgb[3]);
                }
            }
        }
        
        int wrapWidth = loc[2] - loc[0];  // 기본 wrap width는 rect width
        int maxHeight = loc[3] - loc[1];  // 기본 max height는 rect height
        
        // BackgroundTextWidget 생성
        auto bgTextWidget = std::make_unique<BackgroundTextWidget>(
            uiManager, resourceManager, renderer, textRenderer,
            text, size, textColor, bgColor, wrapWidth, maxHeight
        );
        
        // 배경 이미지 설정 (선택)
        if (uiElement.contains("backgroundImage")) {
            std::string bgImage = uiElement["backgroundImage"].get<std::string>();
            bool useNinePatch = uiElement.value("backgroundImageNinePatch", false);
            bool useThreePatch = uiElement.value("backgroundImageThreePatch", false);
            bgTextWidget->setBackgroundImage(bgImage, useNinePatch, useThreePatch);
        }

        // 고정 배경 크기 (fixedSize: true면 loc 크기 사용)
        if (uiElement.value("fixedSize", false)) {
            int locW = loc[2] - loc[0];
            int locH = loc[3] - loc[1];
            if (locW > 0 && locH > 0) {
                bgTextWidget->setFixedBackgroundSize(locW, locH);
            }
        }

        // 텍스트 정렬: 전체 위젯을 loc 내에서 배치 (배경은 텍스트에 맞춤)
        std::string textAlign = "left";
        if (uiElement.contains("textAlign")) {
            textAlign = uiElement["textAlign"].get<std::string>();
            bgTextWidget->setTextAlign(textAlign);
        }
        
        // 배경 UIElement의 위치 및 속성 설정
        std::string bgElementId = bgTextWidget->getBackgroundElementId();
        auto* bgElement = uiManager->findElementByName(bgElementId);
        if (bgElement) {
            int locW = loc[2] - loc[0];
            bgElement->rect.y = loc[1];
            if (textAlign == "right") {
                bgElement->rect.x = loc[2] - bgElement->rect.w;  // 오른쪽 끝 정렬
            } else if (textAlign == "center") {
                bgElement->rect.x = loc[0] + (locW - bgElement->rect.w) / 2;  // 가운데 정렬
            } else {
                bgElement->rect.x = loc[0];
            }
            
            // 선택적 필드 처리
            applyElementProperties(bgElement, uiElement);
            
            // 부모-자식 관계 설정
            applyParentRelationship(bgElementId, uiElement);
        }
        
        addWidget(uiName, std::move(bgTextWidget));
        return true;
    }
    else if (type == "tilemap") {
        nlohmann::json mapJson;
        std::string tilesetName;
        int tileSize, tilesetColumns, mapWidth, mapHeight;
        
        // 맵 파일 사용 여부 확인
        if (uiElement.contains("map")) {
            // ResourceManager에서 맵 파일 로드
            std::string mapName = uiElement["map"].get<std::string>();
            mapJson = resourceManager->getMapJson(mapName);
            if (mapJson.is_null()) {
                Log::error("[WidgetManager] Map not found: ", mapName);
                return false;
            }
            
            // 맵 파일에서 정보 가져오기
            tilesetName = mapJson["tileset"].get<std::string>();
            tileSize = mapJson["tileSize"].get<int>();
            tilesetColumns = mapJson["tilesetColumns"].get<int>();
            mapWidth = mapJson["mapWidth"].get<int>();
            mapHeight = mapJson["mapHeight"].get<int>();
        } else {
            // 직접 지정 방식 (기존 방식, 호환성 유지)
            tilesetName = uiElement["tileset"].get<std::string>();
            tileSize = uiElement["tileSize"].get<int>();
            tilesetColumns = uiElement["tilesetColumns"].get<int>();
            mapWidth = uiElement["mapWidth"].get<int>();
            mapHeight = uiElement["mapHeight"].get<int>();
        }
        
        // TilemapWidget 생성 (rect 및 UIElement 속성 포함)
        SDL_Rect tilemapRect = SDL_Rect{loc[0], loc[1], loc[2] - loc[0], loc[3] - loc[1]};
        float scale = 1.0f;
        float rotation = 0.0f;
        float alpha = 1.0f;
        bool visible = true;
        bool clickable = false;  // 기본값
        
        if (uiElement.contains("scale")) {
            scale = uiElement["scale"].get<float>();
        }
        if (uiElement.contains("rotate")) {
            rotation = uiElement["rotate"].get<float>();
        }
        if (uiElement.contains("alpha")) {
            alpha = uiElement["alpha"].get<float>();
        }
        if (uiElement.contains("visible")) {
            visible = uiElement["visible"].get<bool>();
        }
        if (uiElement.contains("clickable")) {
            clickable = uiElement["clickable"].get<bool>();
        }
        
        auto tilemapWidget = std::make_unique<TilemapWidget>(
            uiManager, resourceManager, renderer,
            tilesetName, tileSize, tilesetColumns, mapWidth, mapHeight,
            tilemapRect, scale, rotation, alpha, visible, clickable
        );
        
        // 타일 데이터 설정
        if (!mapJson.is_null() && mapJson.contains("tileData")) {
            // 맵 파일에서 타일 데이터 가져오기
            std::vector<std::vector<int>> tileData;
            auto dataArray = mapJson["tileData"];
            for (const auto& row : dataArray) {
                std::vector<int> rowData;
                for (const auto& tileId : row) {
                    rowData.push_back(tileId.get<int>());
                }
                tileData.push_back(rowData);
            }
            tilemapWidget->setTileData(tileData);
        } else if (uiElement.contains("tileData")) {
            // 직접 지정 방식 (기존 방식)
            std::vector<std::vector<int>> tileData;
            auto dataArray = uiElement["tileData"];
            for (const auto& row : dataArray) {
                std::vector<int> rowData;
                for (const auto& tileId : row) {
                    rowData.push_back(tileId.get<int>());
                }
                tileData.push_back(rowData);
            }
            tilemapWidget->setTileData(tileData);
        }
        
        // 오프셋 설정 (선택적)
        if (uiElement.contains("offset")) {
            auto offset = uiElement["offset"].get<std::vector<int>>();
            if (offset.size() >= 2) {
                tilemapWidget->setOffset(offset[0], offset[1]);
            }
        }
        
        // 부모-자식 관계 설정
        std::string tilemapElementId = tilemapWidget->getUIElementIdentifier();
        applyParentRelationship(tilemapElementId, uiElement);
        
        addWidget(uiName, std::move(tilemapWidget));
        return true;
    }
    else if (type == "edittext") {
        
        std::string placeholder = "";
        if (uiElement.contains("placeholder")) {
            placeholder = uiElement["placeholder"].get<std::string>();
        }
        
        int fontSize = 16;
        if (uiElement.contains("textSize")) {
            fontSize = uiElement["textSize"].get<int>();
        }
        
        SDL_Color textColor = {255, 255, 255, 255};
        if (uiElement.contains("textColor")) {
            auto rgb = uiElement["textColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                textColor.r = static_cast<Uint8>(rgb[0]);
                textColor.g = static_cast<Uint8>(rgb[1]);
                textColor.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) {
                    textColor.a = static_cast<Uint8>(rgb[3]);
                }
            }
        }
        
        SDL_Color bgColor = {50, 50, 50, 255};
        if (uiElement.contains("backgroundColor")) {
            auto rgb = uiElement["backgroundColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                bgColor.r = static_cast<Uint8>(rgb[0]);
                bgColor.g = static_cast<Uint8>(rgb[1]);
                bgColor.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) {
                    bgColor.a = static_cast<Uint8>(rgb[3]);
                }
            }
        }
        
        SDL_Color borderColor = {100, 100, 100, 255};
        if (uiElement.contains("borderColor")) {
            auto rgb = uiElement["borderColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                borderColor.r = static_cast<Uint8>(rgb[0]);
                borderColor.g = static_cast<Uint8>(rgb[1]);
                borderColor.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) {
                    borderColor.a = static_cast<Uint8>(rgb[3]);
                }
            }
        }
        
        int maxLength = 0;  // 0이면 무제한
        if (uiElement.contains("maxLength")) {
            maxLength = uiElement["maxLength"].get<int>();
        }
        
        bool multiline = false;
        if (uiElement.contains("multiline")) {
            multiline = uiElement["multiline"].get<bool>();
        }
        
        // EditTextWidget 생성 (생성자에서 UIElement도 생성됨)
        auto editTextWidget = std::make_unique<EditTextWidget>(
            uiManager, resourceManager, renderer, textRenderer,
            placeholder, fontSize, textColor, bgColor, borderColor, maxLength, multiline
        );
        
        // UIElement의 위치 및 속성 설정 (EditTextWidget이 자동 생성한 ID 사용)
        std::string elementId = editTextWidget->getUIElementId();
        auto* element = uiManager->findElementByName(elementId);
        if (element) {
            element->rect.x = loc[0];
            element->rect.y = loc[1];
            element->rect.w = loc[2] - loc[0];
            element->rect.h = loc[3] - loc[1];
            
            // 선택적 필드 처리
            applyElementProperties(element, uiElement);
            
            // EditText는 기본적으로 클릭 가능 (JSON에 명시되지 않은 경우)
            if (!uiElement.contains("clickable")) {
                element->clickable = true;
            }
        }
        
        // 부모-자식 관계 설정
        applyParentRelationship(elementId, uiElement);
        
        // 크기 설정 후 텍스처 업데이트 (생성자에서 크기가 0이었을 수 있음)
        editTextWidget->updateTexture();
        
        addWidget(uiName, std::move(editTextWidget));
        return true;
    }
    else if (type == "textlist") {
        
        int itemHeight = 30;
        if (uiElement.contains("itemHeight")) {
            itemHeight = uiElement["itemHeight"].get<int>();
        }
        
        int fontSize = 12;
        if (uiElement.contains("textSize")) {
            fontSize = uiElement["textSize"].get<int>();
        }
        
        SDL_Color color = {255, 255, 255, 255};
        if (uiElement.contains("textColor")) {
            auto rgb = uiElement["textColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                color.r = static_cast<Uint8>(rgb[0]);
                color.g = static_cast<Uint8>(rgb[1]);
                color.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) {
                    color.a = static_cast<Uint8>(rgb[3]);
                }
            }
        }
        
        // TextListWidget 생성 (rect 및 UIElement 속성 포함)
        SDL_Rect listRect = SDL_Rect{loc[0], loc[1], loc[2] - loc[0], loc[3] - loc[1]};
        float scale = 1.0f;
        float rotation = 0.0f;
        float alpha = 1.0f;
        bool visible = true;
        bool clickable = true;  // 드래그를 위해 클릭 가능
        
        if (uiElement.contains("scale")) {
            scale = uiElement["scale"].get<float>();
        }
        if (uiElement.contains("rotate")) {
            rotation = uiElement["rotate"].get<float>();
        }
        if (uiElement.contains("alpha")) {
            alpha = uiElement["alpha"].get<float>();
        }
        if (uiElement.contains("visible")) {
            visible = uiElement["visible"].get<bool>();
        }
        if (uiElement.contains("clickable")) {
            clickable = uiElement["clickable"].get<bool>();
        }
        
        auto textListWidget = std::make_unique<TextListWidget>(
            uiManager, resourceManager, renderer, textRenderer,
            itemHeight, fontSize, color, listRect, scale, rotation, alpha, visible, clickable
        );
        
        // 텍스트 리스트 항목 설정
        if (uiElement.contains("items")) {
            std::vector<std::string> items;
            for (const auto& item : uiElement["items"]) {
                items.push_back(item.get<std::string>());
            }
            textListWidget->setItems(items);
        }
        
        // 부모-자식 관계 설정
        std::string textListElementId = textListWidget->getUIElementIdentifier();
        applyParentRelationship(textListElementId, uiElement);
        
        addWidget(uiName, std::move(textListWidget));
        return true;
    }
    else if (type == "chatlist") {
        
        int itemHeight = 60;
        if (uiElement.contains("itemHeight")) {
            itemHeight = uiElement["itemHeight"].get<int>();
        }
        
        int fontSize = 14;
        if (uiElement.contains("textSize")) {
            fontSize = uiElement["textSize"].get<int>();
        }
        
        SDL_Color color = {255, 255, 255, 255};
        if (uiElement.contains("textColor")) {
            auto rgb = uiElement["textColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                color.r = static_cast<Uint8>(rgb[0]);
                color.g = static_cast<Uint8>(rgb[1]);
                color.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) {
                    color.a = static_cast<Uint8>(rgb[3]);
                }
            }
        }
        
        int iconSize = 40;
        if (uiElement.contains("iconSize")) {
            iconSize = uiElement["iconSize"].get<int>();
        }
        
        int iconTextSpacing = 10;
        if (uiElement.contains("iconTextSpacing")) {
            iconTextSpacing = uiElement["iconTextSpacing"].get<int>();
        }
        
        // ChatListWidget 생성 (rect 및 UIElement 속성 포함)
        SDL_Rect chatRect = SDL_Rect{loc[0], loc[1], loc[2] - loc[0], loc[3] - loc[1]};
        float scale = 1.0f;
        float rotation = 0.0f;
        float alpha = 1.0f;
        bool visible = true;
        bool clickable = true;  // 드래그를 위해 클릭 가능
        
        if (uiElement.contains("scale")) {
            scale = uiElement["scale"].get<float>();
        }
        if (uiElement.contains("rotate")) {
            rotation = uiElement["rotate"].get<float>();
        }
        if (uiElement.contains("alpha")) {
            alpha = uiElement["alpha"].get<float>();
        }
        if (uiElement.contains("visible")) {
            visible = uiElement["visible"].get<bool>();
        }
        if (uiElement.contains("clickable")) {
            clickable = uiElement["clickable"].get<bool>();
        }
        
        auto chatListWidget = std::make_unique<ChatListWidget>(
            uiManager, resourceManager, renderer, textRenderer,
            itemHeight, fontSize, color, iconSize, iconTextSpacing,
            chatRect, scale, rotation, alpha, visible, clickable
        );
        
        // 부모-자식 관계 설정
        std::string chatListElementId = chatListWidget->getUIElementIdentifier();
        applyParentRelationship(chatListElementId, uiElement);
        
        // 위젯 추가
        addWidget(uiName, std::move(chatListWidget));
        
        // 채팅 메시지 설정 (위젯이 추가된 후에 설정)
        if (uiElement.contains("messages")) {
            std::vector<ChatMessage> messages;
            for (const auto& msgJson : uiElement["messages"]) {
                std::string text = msgJson["text"].get<std::string>();
                std::string iconName = "";
                if (msgJson.contains("icon")) {
                    iconName = msgJson["icon"].get<std::string>();
                }
                ChatAlignment alignment = ChatAlignment::LEFT;
                if (msgJson.contains("alignment")) {
                    std::string alignStr = msgJson["alignment"].get<std::string>();
                    if (alignStr == "right" || alignStr == "RIGHT") {
                        alignment = ChatAlignment::RIGHT;
                    }
                }
                messages.push_back(ChatMessage(text, iconName, alignment));
            }
            auto* widget = getWidget(uiName);
            if (widget) {
                auto* chatWidget = dynamic_cast<ChatListWidget*>(widget);
                if (chatWidget) {
                    chatWidget->setMessages(messages);
                }
            }
        }
        
        return true;
    }
    else if (type == "multitypelist") {
        int itemHeight = 60;
        if (uiElement.contains("itemHeight")) {
            itemHeight = uiElement["itemHeight"].get<int>();
        }
        
        int fontSize = 14;
        if (uiElement.contains("textSize")) {
            fontSize = uiElement["textSize"].get<int>();
        }
        
        SDL_Color textColor = {0, 0, 0, 255};
        if (uiElement.contains("textColor")) {
            auto rgb = uiElement["textColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                textColor.r = static_cast<Uint8>(rgb[0]);
                textColor.g = static_cast<Uint8>(rgb[1]);
                textColor.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) {
                    textColor.a = static_cast<Uint8>(rgb[3]);
                }
            }
        }
        
        SDL_Color bgColor = {255, 255, 255, 255};
        if (uiElement.contains("backgroundColor")) {
            auto rgb = uiElement["backgroundColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                bgColor.r = static_cast<Uint8>(rgb[0]);
                bgColor.g = static_cast<Uint8>(rgb[1]);
                bgColor.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) {
                    bgColor.a = static_cast<Uint8>(rgb[3]);
                }
            }
        }
        
        int iconSize = 40;
        if (uiElement.contains("iconSize")) {
            iconSize = uiElement["iconSize"].get<int>();
        }
        
        int iconTextSpacing = 10;
        if (uiElement.contains("iconTextSpacing")) {
            iconTextSpacing = uiElement["iconTextSpacing"].get<int>();
        }
        
        SDL_Rect listRect = SDL_Rect{loc[0], loc[1], loc[2] - loc[0], loc[3] - loc[1]};
        float scale = 1.0f;
        float rotation = 0.0f;
        float alpha = 1.0f;
        bool visible = true;
        bool clickable = true;
        
        if (uiElement.contains("scale")) {
            scale = uiElement["scale"].get<float>();
        }
        if (uiElement.contains("rotate")) {
            rotation = uiElement["rotate"].get<float>();
        }
        if (uiElement.contains("alpha")) {
            alpha = uiElement["alpha"].get<float>();
        }
        if (uiElement.contains("visible")) {
            visible = uiElement["visible"].get<bool>();
        }
        if (uiElement.contains("clickable")) {
            clickable = uiElement["clickable"].get<bool>();
        }
        
        auto multiTypeListWidget = std::make_unique<MultiTypeListWidget>(
            uiManager, resourceManager, renderer, textRenderer,
            itemHeight, fontSize, textColor, bgColor, iconSize, iconTextSpacing,
            listRect, scale, rotation, alpha, visible, clickable
        );
        
        // 부모-자식 관계 설정
        std::string listElementId = multiTypeListWidget->getUIElementIdentifier();
        applyParentRelationship(listElementId, uiElement);
        
        // 위젯 추가
        addWidget(uiName, std::move(multiTypeListWidget));
        
        // 초기 아이템 설정 (위젯이 추가된 후에 설정)
        if (uiElement.contains("items")) {
            std::vector<ListItem> items;
            for (const auto& itemJson : uiElement["items"]) {
                std::string typeStr = itemJson["type"].get<std::string>();
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
                
                std::string text = "";
                if (itemJson.contains("text")) {
                    text = itemJson["text"].get<std::string>();
                }
                std::string iconName = "";
                if (itemJson.contains("iconName")) {
                    iconName = itemJson["iconName"].get<std::string>();
                }
                std::string imageName = "";
                if (itemJson.contains("imageName")) {
                    imageName = itemJson["imageName"].get<std::string>();
                }
                
                items.push_back(ListItem(itemType, text, iconName, imageName));
            }
            auto* widget = getWidget(uiName);
            if (widget) {
                auto* multiTypeWidget = dynamic_cast<MultiTypeListWidget*>(widget);
                if (multiTypeWidget) {
                    multiTypeWidget->setItems(items);
                }
            }
        }
        
        return true;
    }
    else if (type == "upgradelist") {
        int itemHeight = 72;
        if (uiElement.contains("itemHeight")) {
            itemHeight = uiElement["itemHeight"].get<int>();
        }
        int iconSize = 48;
        if (uiElement.contains("iconSize")) {
            iconSize = uiElement["iconSize"].get<int>();
        }
        int iconMargin = 8;
        if (uiElement.contains("iconMargin")) {
            iconMargin = uiElement["iconMargin"].get<int>();
        }
        int titleFontSize = 16;
        if (uiElement.contains("titleFontSize")) {
            titleFontSize = uiElement["titleFontSize"].get<int>();
        }
        int descFontSize = 12;
        if (uiElement.contains("descFontSize")) {
            descFontSize = uiElement["descFontSize"].get<int>();
        }
        int buttonWidth = 80;
        int buttonHeight = 32;
        if (uiElement.contains("buttonWidth")) {
            buttonWidth = uiElement["buttonWidth"].get<int>();
        }
        if (uiElement.contains("buttonHeight")) {
            buttonHeight = uiElement["buttonHeight"].get<int>();
        }
        int buttonFontSize = 14;
        if (uiElement.contains("buttonFontSize")) {
            buttonFontSize = uiElement["buttonFontSize"].get<int>();
        }
        SDL_Color titleColor = {60, 60, 80, 255};
        if (uiElement.contains("titleColor")) {
            auto rgb = uiElement["titleColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                titleColor.r = static_cast<Uint8>(rgb[0]);
                titleColor.g = static_cast<Uint8>(rgb[1]);
                titleColor.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) titleColor.a = static_cast<Uint8>(rgb[3]);
            }
        }
        SDL_Color descColor = {100, 100, 120, 255};
        if (uiElement.contains("descColor")) {
            auto rgb = uiElement["descColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                descColor.r = static_cast<Uint8>(rgb[0]);
                descColor.g = static_cast<Uint8>(rgb[1]);
                descColor.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) descColor.a = static_cast<Uint8>(rgb[3]);
            }
        }
        SDL_Color buttonTextColor = {255, 255, 255, 255};
        if (uiElement.contains("buttonTextColor")) {
            auto rgb = uiElement["buttonTextColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                buttonTextColor.r = static_cast<Uint8>(rgb[0]);
                buttonTextColor.g = static_cast<Uint8>(rgb[1]);
                buttonTextColor.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) buttonTextColor.a = static_cast<Uint8>(rgb[3]);
            }
        }
        SDL_Rect rowRect = SDL_Rect{loc[0], loc[1], loc[2] - loc[0], loc[3] - loc[1]};
        float scale = 1.0f;
        float rotation = 0.0f;
        float alpha = 1.0f;
        bool visible = true;
        bool clickable = true;
        if (uiElement.contains("scale")) scale = uiElement["scale"].get<float>();
        if (uiElement.contains("rotate")) rotation = uiElement["rotate"].get<float>();
        if (uiElement.contains("alpha")) alpha = uiElement["alpha"].get<float>();
        if (uiElement.contains("visible")) visible = uiElement["visible"].get<bool>();
        if (uiElement.contains("clickable")) clickable = uiElement["clickable"].get<bool>();
        
        auto upgradeListWidget = std::make_unique<UpgradeListWidget>(
            uiManager, resourceManager, renderer, textRenderer,
            itemHeight, iconSize, iconMargin,
            titleFontSize, descFontSize,
            buttonWidth, buttonHeight, buttonFontSize,
            titleColor, descColor, buttonTextColor,
            rowRect, scale, rotation, alpha, visible, clickable
        );
        upgradeListWidget->setWidgetManager(this);
        upgradeListWidget->setAnimationManager(animationManager);
        
        std::string listElementId = upgradeListWidget->getUIElementIdentifier();
        applyParentRelationship(listElementId, uiElement);
        
        addWidget(uiName, std::move(upgradeListWidget));
        
        if (uiElement.contains("itemBackgroundImage")) {
            auto* widget = getWidget(uiName);
            if (widget) {
                auto* upgradeWidget = dynamic_cast<UpgradeListWidget*>(widget);
                if (upgradeWidget) {
                    std::string bgImg = uiElement["itemBackgroundImage"].get<std::string>();
                    bool useNinePatch = uiElement.value("itemBackgroundNinePatch", false);
                    bool useThreePatch = uiElement.value("itemBackgroundThreePatch", false);
                    upgradeWidget->setItemBackgroundImage(bgImg, useNinePatch, useThreePatch);
                }
            }
        }
        if (uiElement.contains("buttonNinePatch") || uiElement.contains("buttonThreePatch")) {
            auto* widget = getWidget(uiName);
            if (widget) {
                auto* upgradeWidget = dynamic_cast<UpgradeListWidget*>(widget);
                if (upgradeWidget) {
                    bool useNine = uiElement.value("buttonNinePatch", false);
                    bool useThree = uiElement.value("buttonThreePatch", false);
                    upgradeWidget->setButtonNinePatch(useNine);
                    upgradeWidget->setButtonThreePatch(useThree);
                }
            }
        }
        
        if (uiElement.contains("items")) {
            auto* widget = getWidget(uiName);
            if (widget) {
                auto* upgradeWidget = dynamic_cast<UpgradeListWidget*>(widget);
                if (upgradeWidget) {
                    for (const auto& itemJson : uiElement["items"]) {
                        std::string icon = itemJson.contains("iconName") ? itemJson["iconName"].get<std::string>() : "";
                        std::string title = itemJson.contains("titleText") ? itemJson["titleText"].get<std::string>() : "";
                        std::string desc = itemJson.contains("descText") ? itemJson["descText"].get<std::string>() : "";
                        std::string btnImg = itemJson.contains("buttonImage") ? itemJson["buttonImage"].get<std::string>() : "";
                        std::string btnTxt = itemJson.contains("buttonText") ? itemJson["buttonText"].get<std::string>() : "";
                        upgradeWidget->addItem(icon, title, desc, btnImg, btnTxt);
                    }
                }
            }
        }
        
        return true;
    }
    else if (type == "bannerlist") {
        int itemHeight = 48;
        if (uiElement.contains("itemHeight")) {
            itemHeight = uiElement["itemHeight"].get<int>();
        }
        std::string backgroundImage = "";
        if (uiElement.contains("backgroundImage")) {
            backgroundImage = uiElement["backgroundImage"].get<std::string>();
        }
        SDL_Color backgroundColor = {60, 60, 80, 255};
        if (uiElement.contains("backgroundColor")) {
            auto rgb = uiElement["backgroundColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                backgroundColor.r = static_cast<Uint8>(rgb[0]);
                backgroundColor.g = static_cast<Uint8>(rgb[1]);
                backgroundColor.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) backgroundColor.a = static_cast<Uint8>(rgb[3]);
            }
        }
        int fontSize = 16;
        if (uiElement.contains("textSize")) {
            fontSize = uiElement["textSize"].get<int>();
        }
        SDL_Color textColor = {255, 255, 255, 255};
        if (uiElement.contains("textColor")) {
            auto rgb = uiElement["textColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                textColor.r = static_cast<Uint8>(rgb[0]);
                textColor.g = static_cast<Uint8>(rgb[1]);
                textColor.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) textColor.a = static_cast<Uint8>(rgb[3]);
            }
        }
        BannerTextAlignH alignH = BannerTextAlignH::Center;
        BannerTextAlignV alignV = BannerTextAlignV::Center;
        if (uiElement.contains("textAlignH")) {
            std::string s = uiElement["textAlignH"].get<std::string>();
            if (s == "left") alignH = BannerTextAlignH::Left;
            else if (s == "right") alignH = BannerTextAlignH::Right;
            else alignH = BannerTextAlignH::Center;
        }
        if (uiElement.contains("textAlignV")) {
            std::string s = uiElement["textAlignV"].get<std::string>();
            if (s == "top") alignV = BannerTextAlignV::Top;
            else if (s == "bottom") alignV = BannerTextAlignV::Bottom;
            else alignV = BannerTextAlignV::Center;
        }
        SDL_Rect bannerRect = SDL_Rect{loc[0], loc[1], loc[2] - loc[0], loc[3] - loc[1]};
        float scale = 1.0f;
        float rotation = 0.0f;
        float alpha = 1.0f;
        bool visible = true;
        bool clickable = true;
        if (uiElement.contains("scale")) scale = uiElement["scale"].get<float>();
        if (uiElement.contains("rotate")) rotation = uiElement["rotate"].get<float>();
        if (uiElement.contains("alpha")) alpha = uiElement["alpha"].get<float>();
        if (uiElement.contains("visible")) visible = uiElement["visible"].get<bool>();
        if (uiElement.contains("clickable")) clickable = uiElement["clickable"].get<bool>();

        auto bannerListWidget = std::make_unique<BannerListWidget>(
            uiManager, resourceManager, renderer, textRenderer,
            itemHeight, backgroundImage, backgroundColor,
            fontSize, textColor, alignH, alignV,
            bannerRect, scale, rotation, alpha, visible, clickable
        );

        if (uiElement.contains("itemMargin")) {
            bannerListWidget->setItemMargin(uiElement["itemMargin"].get<int>());
        }

        std::string listElementId = bannerListWidget->getUIElementIdentifier();
        applyParentRelationship(listElementId, uiElement);

        addWidget(uiName, std::move(bannerListWidget));

        if (uiElement.contains("items")) {
            auto* widget = getWidget(uiName);
            if (widget) {
                auto* bannerWidget = dynamic_cast<BannerListWidget*>(widget);
                if (bannerWidget) {
                    for (const auto& itemJson : uiElement["items"]) {
                        std::string title, imageName;
                        if (itemJson.is_string()) {
                            title = itemJson.get<std::string>();
                        } else if (itemJson.is_object()) {
                            if (itemJson.contains("title")) title = itemJson["title"].get<std::string>();
                            if (itemJson.contains("imageName")) imageName = itemJson["imageName"].get<std::string>();
                        }
                        bannerWidget->addItem(title, imageName);
                    }
                }
            }
        }

        return true;
    }
    else if (type == "verticalgrid") {
        int columns = 2;
        if (uiElement.contains("columns")) {
            columns = uiElement["columns"].get<int>();
        }
        int cellWidth = 100;
        int cellHeight = 100;
        if (uiElement.contains("cellWidth")) {
            cellWidth = uiElement["cellWidth"].get<int>();
        }
        if (uiElement.contains("cellHeight")) {
            cellHeight = uiElement["cellHeight"].get<int>();
        }
        std::string cellBackgroundImage = "";
        if (uiElement.contains("cellBackgroundImage")) {
            cellBackgroundImage = uiElement["cellBackgroundImage"].get<std::string>();
        }
        int cellMargin = 8;
        if (uiElement.contains("cellMargin")) {
            cellMargin = uiElement["cellMargin"].get<int>();
        }
        
        SDL_Rect gridRect = SDL_Rect{loc[0], loc[1], loc[2] - loc[0], loc[3] - loc[1]};
        float scale = 1.0f;
        float rotation = 0.0f;
        float alpha = 1.0f;
        bool visible = true;
        bool clickable = true;
        
        if (uiElement.contains("scale")) scale = uiElement["scale"].get<float>();
        if (uiElement.contains("rotate")) rotation = uiElement["rotate"].get<float>();
        if (uiElement.contains("alpha")) alpha = uiElement["alpha"].get<float>();
        if (uiElement.contains("visible")) visible = uiElement["visible"].get<bool>();
        if (uiElement.contains("clickable")) clickable = uiElement["clickable"].get<bool>();
        
        auto gridWidget = std::make_unique<VerticalGridWidget>(
            uiManager, resourceManager,
            columns, cellWidth, cellHeight,
            gridRect, cellBackgroundImage, cellMargin, scale, rotation, alpha, visible, clickable
        );
        
        if (uiElement.contains("items")) {
            for (const auto& item : uiElement["items"]) {
                std::string imgName = item.get<std::string>();
                gridWidget->addItem(imgName);
            }
        }
        
        std::string gridElementId = gridWidget->getUIElementIdentifier();
        applyParentRelationship(gridElementId, uiElement);
        
        addWidget(uiName, std::move(gridWidget));
        return true;
    }
    else if (type == "sectiongrid") {
        int columns = 2;
        if (uiElement.contains("columns")) {
            columns = uiElement["columns"].get<int>();
        }
        int cellWidth = 100;
        int cellHeight = 100;
        if (uiElement.contains("cellWidth")) {
            cellWidth = uiElement["cellWidth"].get<int>();
        }
        if (uiElement.contains("cellHeight")) {
            cellHeight = uiElement["cellHeight"].get<int>();
        }
        int cellMargin = 8;
        if (uiElement.contains("cellMargin")) {
            cellMargin = uiElement["cellMargin"].get<int>();
        }
        int headerMarginV = 8;
        if (uiElement.contains("headerMarginV")) {
            headerMarginV = uiElement["headerMarginV"].get<int>();
        }
        
        SDL_Rect gridRect = SDL_Rect{loc[0], loc[1], loc[2] - loc[0], loc[3] - loc[1]};
        float scale = 1.0f;
        float rotation = 0.0f;
        float alpha = 1.0f;
        bool visible = true;
        bool clickable = true;
        
        if (uiElement.contains("scale")) scale = uiElement["scale"].get<float>();
        if (uiElement.contains("rotate")) rotation = uiElement["rotate"].get<float>();
        if (uiElement.contains("alpha")) alpha = uiElement["alpha"].get<float>();
        if (uiElement.contains("visible")) visible = uiElement["visible"].get<bool>();
        if (uiElement.contains("clickable")) clickable = uiElement["clickable"].get<bool>();
        
        auto sectionGrid = std::make_unique<SectionGridWidget>(
            uiManager, resourceManager, renderer, textRenderer,
            columns, cellWidth, cellHeight,
            gridRect, cellMargin, headerMarginV,
            scale, rotation, alpha, visible, clickable
        );
        
        std::string rootId = sectionGrid->getUIElementIdentifier();
        applyParentRelationship(rootId, uiElement);
        
        addWidget(uiName, std::move(sectionGrid));
        return true;
    }
    else if (type == "background") {
        // BackgroundWidget 생성
        SDL_Rect bgRect = SDL_Rect{loc[0], loc[1], loc[2] - loc[0], loc[3] - loc[1]};
        auto bgWidget = std::make_unique<BackgroundWidget>(
            uiManager, resourceManager, renderer, bgRect, animationManager
        );
        
        // 색상 배경 설정
        if (uiElement.contains("bgColor")) {
            auto rgb = uiElement["bgColor"].get<std::vector<int>>();
            SDL_Color color{0, 0, 0, 255};
            if (rgb.size() >= 3) {
                color.r = static_cast<Uint8>(rgb[0]);
                color.g = static_cast<Uint8>(rgb[1]);
                color.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() >= 4) {
                    color.a = static_cast<Uint8>(rgb[3]);
                }
            }
            bgWidget->setBackgroundColor(color);
        }
        
        // 이미지 배경 설정
        if (uiElement.contains("image")) {
            std::string imgName = uiElement["image"].get<std::string>();
            std::string modeStr = "stretch";  // 기본값
            if (uiElement.contains("imageMode")) {
                modeStr = uiElement["imageMode"].get<std::string>();
            }
            
            BackgroundImageMode mode = BackgroundImageMode::STRETCH;
            if (modeStr == "tile") {
                mode = BackgroundImageMode::TILE;
            } else if (modeStr == "fit") {
                mode = BackgroundImageMode::FIT;
            } else if (modeStr == "scroll_h") {
                mode = BackgroundImageMode::SCROLL_H;
            } else if (modeStr == "scroll_v") {
                mode = BackgroundImageMode::SCROLL_V;
            }
            bgWidget->setBackgroundImage(imgName, mode);

            if (mode == BackgroundImageMode::SCROLL_H || mode == BackgroundImageMode::SCROLL_V) {
                if (uiElement.contains("scrollCycleTime") || uiElement.contains("scrollDuration")) {
                    std::string key = uiElement.contains("scrollDuration") ? "scrollDuration" : "scrollCycleTime";
                    float sec = uiElement[key].is_number()
                        ? static_cast<float>(uiElement[key].get<double>()) : 10.0f;
                    bgWidget->setScrollDuration(sec);
                }
            }
        }
        
        // 위젯 속성 설정
        std::string bgElementId = bgWidget->getUIElementIdentifier();
        auto* element = uiManager->findElementByName(bgElementId);
        if (element) {
            applyElementProperties(element, uiElement);
        }
        
        // 부모-자식 관계 설정
        applyParentRelationship(bgElementId, uiElement);
        
        addWidget(uiName, std::move(bgWidget));
        return true;
    }
    else if (type == "dialog") {
        // StandardDialogWidget 생성 (WidgetManager를 전달하여 내부 버튼 등록 가능하게)
        auto dialogWidget = std::make_unique<StandardDialogWidget>(
            uiManager, resourceManager, renderer, textRenderer, animationManager, this
        );
        
        // 배경 크기 설정 (JSON에서 지정된 경우)
        std::string bgElementId = dialogWidget->getUIElementIdentifier();
        auto* bgElement = uiManager->findElementByName(bgElementId);
        if (bgElement && loc.size() >= 4) {
            bgElement->rect.w = loc[2] - loc[0];
            bgElement->rect.h = loc[3] - loc[1];
        }
        
        // 위젯 추가 (내부 버튼들은 이미 WidgetManager에 등록됨)
        addWidget(uiName, std::move(dialogWidget));
        
        return true;
    }
    else if (type == "customdialog") {
        // CustomDialogWidget 생성
        std::string bgImageName = "";
        if (uiElement.contains("image")) {
            bgImageName = uiElement["image"].get<std::string>();
        }
        
        SDL_Rect dialogRect = SDL_Rect{loc[0], loc[1], loc[2] - loc[0], loc[3] - loc[1]};
        
        // 위젯 이름을 전달하여 배경 UIElement 이름을 위젯 이름과 동일하게 설정
        auto customDialogWidget = std::make_unique<CustomDialogWidget>(
            uiManager, resourceManager, renderer, animationManager,
            uiName, bgImageName, dialogRect
        );
        
        // 위젯 속성 설정
        std::string dialogElementId = customDialogWidget->getUIElementIdentifier();
        auto* dialogElement = uiManager->findElementByName(dialogElementId);
        if (dialogElement) {
            applyElementProperties(dialogElement, uiElement);
        }
        
        // 위젯 추가
        addWidget(uiName, std::move(customDialogWidget));
        
        return true;
    }
    else if (type == "toast") {
        // ToastWidget 생성
        SDL_Rect toastRect = SDL_Rect{loc[0], loc[1], loc[2] - loc[0], loc[3] - loc[1]};
        
        int fontSize = 18;
        if (uiElement.contains("textSize")) {
            fontSize = uiElement["textSize"].get<int>();
        }
        
        SDL_Color textColor = {0, 0, 0, 255};
        if (uiElement.contains("textColor")) {
            auto rgb = uiElement["textColor"].get<std::vector<int>>();
            if (rgb.size() >= 3) {
                textColor.r = static_cast<Uint8>(rgb[0]);
                textColor.g = static_cast<Uint8>(rgb[1]);
                textColor.b = static_cast<Uint8>(rgb[2]);
                if (rgb.size() == 4) {
                    textColor.a = static_cast<Uint8>(rgb[3]);
                }
            }
        }
        
        auto toastWidget = std::make_unique<ToastWidget>(
            uiManager, resourceManager, renderer, textRenderer, animationManager,
            toastRect, fontSize, textColor
        );
        
        // 위젯 속성 설정
        std::string toastElementId = toastWidget->getUIElementIdentifier();
        auto* toastElement = uiManager->findElementByName(toastElementId);
        if (toastElement) {
            applyElementProperties(toastElement, uiElement);
        }
        
        // 부모-자식 관계 설정
        applyParentRelationship(toastElementId, uiElement);
        
        addWidget(uiName, std::move(toastWidget));
        
        return true;
    }
    
    // 알 수 없는 위젯 타입
    Log::error("[WidgetManager] Unknown widget type: ", type, " for ", uiName);
    return false;
}

