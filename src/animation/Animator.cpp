#include "Animator.h"
#include "../ui/uiManager.h"  // UIElement 사용
#include "../utils/logger.h"

Animator::Animator(const std::string& targetName)
    : targetName(targetName) {}

void Animator::applyInitialStateFromUI(const UIElement& element) {
    if (!currentStep || !currentStep->startDeferred) return;

    currentStep->startDeferred = false;

    currentStep->fromX = static_cast<float>(element.rect.x);
    currentStep->fromY = static_cast<float>(element.rect.y);
    currentStep->fromWidth = element.rect.w;
    currentStep->fromHeight = element.rect.h;
    currentStep->fromDegree = element.rotation;
    currentStep->fromAlpha = element.alpha;
    currentStep->fromScale = element.scale;
}

void Animator::setInitialStateFromUI(const UIElement& element) {
    // Animator의 초기 상태를 UI 요소의 현재 상태로 설정
    posX = element.rect.x;
    posY = element.rect.y;
    width = element.rect.w;
    height = element.rect.h;
    rotation = element.rotation;
    currentAlpha = element.alpha;
    currentScale = element.scale;
}

Animator& Animator::changeTexture(const std::string& imageName) {
    auto step = std::make_shared<AnimationStep>();
    step->type = AnimationStep::CHANGE_TEXTURE;
    step->imageName = imageName;
    steps.push(step);
    originalSteps.push(step);
    return *this;
}

Animator& Animator::rotate(float endDegree, int durationMs) {
    auto step = std::make_shared<AnimationStep>();
    step->type = AnimationStep::ROTATE;
    step->fromDegree = 0;
    step->degree = endDegree;
    step->duration = durationMs;
    step->startDeferred = true;
    steps.push(step);
    originalSteps.push(step);

    return *this;
}

Animator& Animator::rotate(float startDegree, float endDegree, int durationMs) {
    auto step = std::make_shared<AnimationStep>();
    step->type = AnimationStep::ROTATE;
    step->fromDegree = startDegree;
    step->degree = endDegree;
    step->duration = durationMs;
    steps.push(step);
    originalSteps.push(step);

    return *this;
}

Animator& Animator::move(int dx, int dy, int durationMs) {
    auto step = std::make_shared<AnimationStep>();
    step->type = AnimationStep::MOVE;
    step->x = dx;
    step->y = dy;
    step->duration = durationMs;
    step->startDeferred = true;
    steps.push(step);
    originalSteps.push(step);
    return *this;
}

Animator& Animator::moveTo(int endX, int endY, int durationMs) {
    auto step = std::make_shared<AnimationStep>();
    step->type = AnimationStep::MOVE_TO;
    step->fromX = 0;
    step->fromY = 0;
    step->x = endX;
    step->y = endY;
    step->duration = durationMs;
    step->startDeferred = true;
    steps.push(step);
    originalSteps.push(step);

    return *this;
}

Animator& Animator::moveTo(int startX, int startY, int endX, int endY, int durationMs) {
    auto step = std::make_shared<AnimationStep>();
    step->type = AnimationStep::MOVE_TO;
    step->fromX = startX;
    step->fromY = startY;
    step->x = endX;
    step->y = endY;
    step->duration = durationMs;
    steps.push(step);
    originalSteps.push(step);

    return *this;
}

Animator& Animator::resize(int endW, int endH, int durationMs) {
    auto step = std::make_shared<AnimationStep>();
    step->type = AnimationStep::RESIZE;
    step->fromWidth = 0;
    step->fromHeight = 0;
    step->width = endW;
    step->height = endH;
    step->duration = durationMs;
    step->startDeferred = true;
    steps.push(step);
    originalSteps.push(step);

    return *this;
}

Animator& Animator::resize(int startW, int startH, int endW, int endH, int durationMs) {
    auto step = std::make_shared<AnimationStep>();
    step->type = AnimationStep::RESIZE;
    step->fromWidth = startW;
    step->fromHeight = startH;
    step->width = endW;
    step->height = endH;
    step->duration = durationMs;
    steps.push(step);
    originalSteps.push(step);

    return *this;
}

Animator& Animator::alpha(float endAlpha, int durationMs) {
    auto step = std::make_shared<AnimationStep>();
    step->type = AnimationStep::ALPHA;
    step->fromAlpha = 0.0f;  // Will be set from UI
    step->alpha = endAlpha;
    step->duration = durationMs;
    step->startDeferred = true;
    steps.push(step);
    originalSteps.push(step);
    return *this;
}

Animator& Animator::alpha(float startAlpha, float endAlpha, int durationMs) {
    auto step = std::make_shared<AnimationStep>();
    step->type = AnimationStep::ALPHA;
    step->fromAlpha = startAlpha;
    step->alpha = endAlpha;
    step->duration = durationMs;
    steps.push(step);
    originalSteps.push(step);
    return *this;
}

Animator& Animator::scale(float endScale, int durationMs) {
    auto step = std::make_shared<AnimationStep>();
    step->type = AnimationStep::SCALE;
    step->fromScale = 0.0f;  // Will be set from UI
    step->scale = endScale;
    step->duration = durationMs;
    step->startDeferred = true;
    steps.push(step);
    originalSteps.push(step);
    return *this;
}

Animator& Animator::scale(float startScale, float endScale, int durationMs) {
    auto step = std::make_shared<AnimationStep>();
    step->type = AnimationStep::SCALE;
    step->fromScale = startScale;
    step->scale = endScale;
    step->duration = durationMs;
    steps.push(step);
    originalSteps.push(step);
    return *this;
}

Animator& Animator::delay(int durationMs) {
    auto step = std::make_shared<AnimationStep>();
    step->type = AnimationStep::DELAY;
    step->duration = durationMs;
    steps.push(step);
    originalSteps.push(step);
    return *this;
}

Animator& Animator::repeat(int count) {
    repeatCount = count;
    
    return *this;
}

Animator& Animator::callback(Callback cb) {
    auto step = std::make_shared<AnimationStep>();
    step->type = AnimationStep::CALLBACK;
    step->cb = cb;
    steps.push(step);
    originalSteps.push(step);
    return *this;
}

Animator& Animator::setVisible(bool visible) {
    auto step = std::make_shared<AnimationStep>();
    step->type = AnimationStep::SET_VISIBLE;
    step->visibleValue = visible;
    step->duration = 0;
    steps.push(step);
    originalSteps.push(step);
    return *this;
}

// bool 로 바꾸고, false 면 manager 에서 얘를 제거 해야 한다.
void Animator::update(float deltaTime) {
    if (!currentStep && !steps.empty()) {
        currentStep = steps.front();
        steps.pop();
        currentStep->elapsed = 0;

        // 💡 startDeferred 처리
        if (currentStep->startDeferred) {
            currentStep->fromX = static_cast<float>(posX);
            currentStep->fromY = static_cast<float>(posY);
            currentStep->fromDegree = rotation;
            currentStep->fromWidth = width;
            currentStep->fromHeight = height;
            currentStep->fromAlpha = currentAlpha;
            currentStep->fromScale = currentScale;
            currentStep->startDeferred = false;
        }

        // 텍스처 즉시 반영
        if (currentStep->type == AnimationStep::CHANGE_TEXTURE) {
            currentImage = currentStep->imageName;
            currentStep = nullptr;
        }
        // visible 즉시 반영
        if (currentStep->type == AnimationStep::SET_VISIBLE) {
            visibleOverrideValid = true;
            visibleOverrideValue = currentStep->visibleValue;
            currentStep = nullptr;
        }
    }

    if (!currentStep) return;

    currentStep->elapsed += static_cast<int>(deltaTime);
    float t = std::min(1.0f, currentStep->duration > 0
        ? static_cast<float>(currentStep->elapsed) / currentStep->duration
        : 1.0f);

    switch (currentStep->type) {
        case AnimationStep::ROTATE: {
            rotation = currentStep->fromDegree +
                       (currentStep->degree - currentStep->fromDegree) * t;
            break;
        }

        case AnimationStep::MOVE: {
            posX = static_cast<int>(currentStep->fromX +
                    currentStep->x * t);  // x = delta 이동량
            posY = static_cast<int>(currentStep->fromY +
                    currentStep->y * t);
            break;
        }

        case AnimationStep::MOVE_TO: {
            posX = static_cast<int>(currentStep->fromX +
                    (currentStep->x - currentStep->fromX) * t);
            posY = static_cast<int>(currentStep->fromY +
                    (currentStep->y - currentStep->fromY) * t);
            break;
        }

        case AnimationStep::RESIZE: {
            width = static_cast<int>(currentStep->fromWidth +
                     (currentStep->width - currentStep->fromWidth) * t);
            height = static_cast<int>(currentStep->fromHeight +
                      (currentStep->height - currentStep->fromHeight) * t);
            break;
        }

        case AnimationStep::ALPHA: {
            currentAlpha = currentStep->fromAlpha +
                    (currentStep->alpha - currentStep->fromAlpha) * t;
            break;
        }

        case AnimationStep::SCALE: {
            currentScale = currentStep->fromScale +
                    (currentStep->scale - currentStep->fromScale) * t;
            break;
        }

        // case AnimationStep::REPEAT:  // todo: 악보의 도돌이표처러 동작하게 수정

        case AnimationStep::DELAY:
            // 아무것도 하지 않음
            break;

        case AnimationStep::CALLBACK:
            if (currentStep->cb) {
                currentStep->cb();
            }
            break;

        default: break;
    }

    if (currentStep->elapsed >= currentStep->duration) {
        currentStep = nullptr;

        if (steps.empty()) {
            if (repeatCount == -1 || repeatCount > 0) {
                if (repeatCount > 0) --repeatCount;

                std::queue<std::shared_ptr<AnimationStep>> copy = originalSteps;
                while (!copy.empty()) {
                    steps.push(copy.front());
                    copy.pop();
                }
            }
        }
    }
}




bool Animator::isFinished() const {
    return currentStep == nullptr && steps.empty() && repeatCount == 0;
}

// 상태 getter
const std::string& Animator::getTargetName() const {
    return targetName;
}

const std::string& Animator::getImageName() const {
    return currentImage;
}

int Animator::getX() const {
    return posX;
}

int Animator::getY() const {
    return posY;
}

int Animator::getWidth() const {
    return width;
}

int Animator::getHeight() const {
    return height;
}

float Animator::getRotation() const {
    return rotation;
}

float Animator::getAlpha() const {
    return currentAlpha;
}

float Animator::getScale() const {
    return currentScale;
}

std::shared_ptr<Animator::AnimationStep> Animator::getCurrentStep() const {
    return currentStep;
}

bool Animator::isAffectingPosition() const {
    return currentStep && (currentStep->type == AnimationStep::MOVE || 
                          currentStep->type == AnimationStep::MOVE_TO);
}

bool Animator::isAffectingSize() const {
    return currentStep && (currentStep->type == AnimationStep::RESIZE);
}

bool Animator::isAffectingRotation() const {
    return currentStep && (currentStep->type == AnimationStep::ROTATE);
}

bool Animator::isAffectingAlpha() const {
    return currentStep && (currentStep->type == AnimationStep::ALPHA);
}

bool Animator::isAffectingScale() const {
    return currentStep && (currentStep->type == AnimationStep::SCALE);
}

bool Animator::consumeVisibleOverride(bool& outVisible) {
    if (!visibleOverrideValid) return false;
    outVisible = visibleOverrideValue;
    visibleOverrideValid = false;
    return true;
}