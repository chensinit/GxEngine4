#include "AnimationManager.h"
#include "../resource/resourceManager.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <iostream>

void AnimationManager::setUiManager(UiManager* uiMgr) {
    this->uiManager = uiMgr;
}

void AnimationManager::setResourceManager(ResourceManager* resMgr) {
    this->resourceManager = resMgr;
}

void AnimationManager::add(const std::string& uiName, std::shared_ptr<Animator> animator) {
    // 에러 체크: Animator가 null인지 확인
    if (!animator) {
        std::cerr << "[ERROR] AnimationManager::add(): Animator is null for UI element '" << uiName << "'!" << std::endl;
        return;
    }
    
    // 에러 체크: UiManager가 설정되어 있는지 확인
    if (!uiManager) {
        std::cerr << "[ERROR] AnimationManager::add(): UiManager is not set!" << std::endl;
        return;
    }
    
    // 에러 체크: UI 요소를 찾을 수 있는지 확인
    const UIElement* elem = uiManager->findElementByName(uiName);
    if (!elem) {
        std::cerr << "[ERROR] AnimationManager::add(): UI element '" << uiName << "' not found!" << std::endl;
        std::cerr << "  Make sure the UI element exists in the scene JSON file." << std::endl;
        return;
    }
    
    // Animator의 초기 상태를 UI 요소의 현재 상태로 설정
    animator->setInitialStateFromUI(*elem);
    
    animators.emplace_back(uiName, animator);
}

void AnimationManager::remove(const std::string& uiName) {
    // 해당 UI 요소의 모든 애니메이션 제거
    animators.erase(
        std::remove_if(animators.begin(), animators.end(),
            [&uiName](const std::pair<std::string, std::shared_ptr<Animator>>& pair) {
                return pair.first == uiName;
            }),
        animators.end()
    );
}

void AnimationManager::update(float deltaTime) {
    if (animators.empty()) return;
    
    // 콜백이 animation.add()/loadFromJson()을 호출하면 animators 벡터가 재할당되어
    // 반복자가 무효화되므로, 복사본을 순회하고 종료된 것만 나중에 실제 목록에서 제거한다.
    using Pair = std::pair<std::string, std::shared_ptr<Animator>>;
    std::vector<Pair> snapshot(animators.begin(), animators.end());
    
    for (Pair& pair : snapshot) {
        const std::string& name = pair.first;
        std::shared_ptr<Animator>& animator = pair.second;

        // 🔄 애니메이션 갱신 (내부에서 Lua 콜백 호출 가능 → animators 변경 가능)
        animator->update(deltaTime);

        // 🔧 UI에 반영
        if (uiManager) {
            if (!animator->getImageName().empty())
                uiManager->changeTexture(name, animator->getImageName());

            if (animator->isAffectingPosition()) {
                uiManager->moveTo(name, animator->getX(), animator->getY());
            }

            if (animator->isAffectingSize())
                uiManager->resize(name, animator->getWidth(), animator->getHeight());

            if (animator->isAffectingRotation())
                uiManager->setRotate(name, animator->getRotation());
            
            if (animator->isAffectingAlpha())
                uiManager->setAlpha(name, animator->getAlpha());
            
            if (animator->isAffectingScale())
                uiManager->setScale(name, animator->getScale());

            bool visibleVal;
            if (animator->consumeVisibleOverride(visibleVal))
                uiManager->setVisible(name, visibleVal);
        }
    }
    
    // 🧹 종료된 애니메이터만 실제 목록에서 제거 (같은 name의 다른 인스턴스와 구분 위해 shared_ptr 비교)
    for (const Pair& pair : snapshot) {
        if (pair.second->isFinished()) {
            auto it = std::find_if(animators.begin(), animators.end(),
                [&pair](const Pair& p) { return p.first == pair.first && p.second == pair.second; });
            if (it != animators.end())
                animators.erase(it);
        }
    }
}

void AnimationManager::clear() {
    animators.clear();
}

bool AnimationManager::loadAnimatorFromJson(const std::string& uiElementName, const std::string& animJsonName) {
    if (!resourceManager) {
        std::cerr << "[ERROR] AnimationManager::loadAnimatorFromJson(): ResourceManager is not set!" << std::endl;
        return false;
    }
    
    if (uiElementName.empty()) {
        std::cerr << "[ERROR] AnimationManager::loadAnimatorFromJson(): UI element name is empty!" << std::endl;
        return false;
    }
    
    // JSON 로드
    nlohmann::json animJson = resourceManager->getAnimationJson(animJsonName);
    if (animJson.is_null() || !animJson.is_array()) {
        std::cerr << "[ERROR] AnimationManager::loadAnimatorFromJson(): Failed to load animation JSON: " << animJsonName << std::endl;
        return false;
    }
    
    // Animator 생성
    auto animator = std::make_shared<Animator>(uiElementName);
    
    // JSON 스텝들을 Animator에 추가
    for (auto& step : animJson) {
        std::string type = step.value("type", "");
        
        if (type == "scale") {
            float fromScale = step.value("fromScale", 1.0f);
            float toScale = step.value("toScale", 1.0f);
            int durationMs = step.value("durationMs", 0);
            animator->scale(fromScale, toScale, durationMs);
        }
        else if (type == "rotate") {
            float fromDegree = step.value("fromDegree", 0.0f);
            float toDegree = step.value("toDegree", 0.0f);
            int durationMs = step.value("durationMs", 0);
            animator->rotate(fromDegree, toDegree, durationMs);
        }
        else if (type == "move") {
            int moveX = step.value("moveX", 0);
            int moveY = step.value("moveY", 0);
            int durationMs = step.value("durationMs", 0);
            animator->move(moveX, moveY, durationMs);
        }
        else if (type == "moveTo") {
            if (step.contains("fromX") && step.contains("fromY")) {
                // 시작/끝 값 버전
                int fromX = step.value("fromX", 0);
                int fromY = step.value("fromY", 0);
                int toX = step.value("toX", 0);
                int toY = step.value("toY", 0);
                int durationMs = step.value("durationMs", 0);
                animator->moveTo(fromX, fromY, toX, toY, durationMs);
            } else {
                // 단일 값 버전 (현재 위치에서 시작) - 하지만 JSON에서는 from/to 모두 명시하므로 이 경우는 없을 것
                int toX = step.value("toX", 0);
                int toY = step.value("toY", 0);
                int durationMs = step.value("durationMs", 0);
                animator->moveTo(toX, toY, durationMs);
            }
        }
        else if (type == "resize") {
            int fromW = step.value("fromW", 0);
            int fromH = step.value("fromH", 0);
            int toW = step.value("toW", 0);
            int toH = step.value("toH", 0);
            int durationMs = step.value("durationMs", 0);
            animator->resize(fromW, fromH, toW, toH, durationMs);
        }
        else if (type == "alpha") {
            float fromAlpha = step.value("fromAlpha", 1.0f);
            float toAlpha = step.value("toAlpha", 1.0f);
            int durationMs = step.value("durationMs", 0);
            animator->alpha(fromAlpha, toAlpha, durationMs);
        }
        else if (type == "changeTexture") {
            std::string image = step.value("image", "");
            if (!image.empty()) {
                animator->changeTexture(image);
            }
        }
        else if (type == "delay") {
            int durationMs = step.value("durationMs", 0);
            animator->delay(durationMs);
        }
        else if (type == "setVisible" || type == "visible") {
            bool visible = step.value("visible", true);
            animator->setVisible(visible);
        }
        else if (type == "repeat") {
            int count = step.value("count", 1);
            animator->repeat(count);
        }
        // callback은 JSON에서 지원하지 않음 (Lua 함수를 전달할 수 없음)
    }
    
    // AnimationManager에 추가
    add(uiElementName, animator);
    return true;
}

