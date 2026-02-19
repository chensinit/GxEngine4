#pragma once

#include <vector>
#include <memory>
#include <string>
#include "Animator.h"  // 같은 폴더
#include "../ui/uiManager.h"

class UiManager; // forward declaration
class ResourceManager; // forward declaration

class AnimationManager {
public:
    AnimationManager() = default;

    void setUiManager(UiManager* uiManager);
    void setResourceManager(ResourceManager* resourceManager);
    void add(const std::string& uiName, std::shared_ptr<Animator> animator);
    void remove(const std::string& uiName); // 특정 UI 요소의 애니메이션 제거
    void update(float deltaTime); // 매 프레임 호출
    void clear(); // 모든 애니메이션 제거 (씬 전환 시 사용)
    
    // JSON에서 Animator 생성 및 추가
    bool loadAnimatorFromJson(const std::string& uiElementName, const std::string& animJsonName);

private:
    std::vector<std::pair<std::string, std::shared_ptr<Animator>>> animators;
    UiManager* uiManager = nullptr; // 기본은 null
    ResourceManager* resourceManager = nullptr;
};
