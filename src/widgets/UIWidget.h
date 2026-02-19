#pragma once
#include <string>
#include "../utils/sdl_includes.h"

class UiManager;
class ResourceManager;
class TextRenderer;
class ImageRenderer;

class UIWidget {
protected:
    UiManager* uiManager;
    ResourceManager* resourceManager;
    
public:
    // 각 위젯이 자신의 UIElement ID/이름을 반환 (순수 가상 함수)
    virtual std::string getUIElementIdentifier() const = 0;
    UIWidget(UiManager* uiMgr, ResourceManager* resMgr);
    virtual ~UIWidget() = default;
    
    // 가상 함수들
    virtual void update(float deltaTime) {}
    virtual void handleEvent(const SDL_Event& event) {}  // SDL 이벤트 처리 (포커스가 있는 위젯의 입력 처리용 등)
    virtual void render(SDL_Renderer* renderer) {}  // 필요시만 오버라이드
    
    // 포커스 관련 (EditText 등에서 오버라이드)
    virtual void setFocus(bool focus) {}  // 포커스 설정/해제
    virtual bool canReceiveFocus() const { return false; }  // 포커스를 받을 수 있는지
    
    // 클릭 관련 (Button 등에서 오버라이드)
    virtual bool canReceiveClick() const { return false; }  // 클릭을 받을 수 있는지
    virtual void onClick() {}  // 클릭 시 호출됨 (WidgetManager가 호출)
    
    // 포인트가 위젯 내부인지 확인 (월드 좌표 기반 공통 구현)
    virtual bool isPointInside(int x, int y) const;
    
    // 위젯이 UIElement를 생성/제어할 때 사용
    UiManager* getUiManager() { return uiManager; }
    ResourceManager* getResourceManager() { return resourceManager; }
};

