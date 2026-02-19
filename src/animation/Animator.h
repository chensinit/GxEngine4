#pragma once
#include <string>
#include <functional>
#include <queue>
#include <memory>

#include "../ui/uiManager.h"


class Animator {
    public:
        Animator(const std::string& targetName);
        using Callback = std::function<void()>;

        struct AnimationStep {
            enum Type { CHANGE_TEXTURE, ROTATE, MOVE, MOVE_TO, RESIZE, ALPHA, SCALE, REPEAT, DELAY, CALLBACK, SET_VISIBLE } type;
            std::string imageName;
            float fromX = 0, fromY = 0, fromDegree = 0;
            float x = 0, y = 0, degree = 0;
            int width = 0, height = 0;
            int fromWidth = 0, fromHeight = 0;
            float fromAlpha = 0.0f, alpha = 0.0f;
            float fromScale = 0.0f, scale = 0.0f;
            int duration = 0;
            int elapsed = 0;
            int value = 0;
            bool visibleValue = false;
            Callback cb;
            bool startDeferred = false;
        };

        void applyInitialStateFromUI(const UIElement& element);
        void setInitialStateFromUI(const UIElement& element);
        Animator& changeTexture(const std::string& imageName);
        Animator& rotate(float degree, int durationMs);
        Animator& rotate(float startDegree, float endDegree, int durationMs);
        Animator& move(int dx, int dy, int durationMs);
        Animator& moveTo(int x, int y, int durationMs);
        Animator& moveTo(int startX, int startY, int endX, int endY, int durationMs);
        Animator& resize(int width, int height, int durationMs);
        Animator& resize(int startW, int startH, int endW, int endH, int durationMs);
        Animator& alpha(float endAlpha, int durationMs);
        Animator& alpha(float startAlpha, float endAlpha, int durationMs);
        Animator& scale(float endScale, int durationMs);
        Animator& scale(float startScale, float endScale, int durationMs);
        Animator& delay(int durationMs);
        Animator& repeat(int count); // -1 for infinite
        Animator& callback(std::function<void()> cb);
        Animator& setVisible(bool visible);
    
        void update(float deltaTime);
        bool isFinished() const;
    
        // 현재 상태 정보
        const std::string& getTargetName() const;
        const std::string& getImageName() const;
        int getX() const;
        int getY() const;
        int getWidth() const;
        int getHeight() const;
        float getRotation() const;
        float getAlpha() const;
        float getScale() const;
        std::shared_ptr<Animator::AnimationStep> getCurrentStep() const;

        bool isAffectingPosition() const;
        bool isAffectingSize() const;
        bool isAffectingRotation() const;
        bool isAffectingAlpha() const;
        bool isAffectingScale() const;

        // SET_VISIBLE 스텝용: 적용 후 true 반환, 내부 플래그 클리어
        bool consumeVisibleOverride(bool& outVisible);

    
        bool operator==(const Animator&) const { // lua 등록용 더미
            return false;
        }
    private:


        std::string targetName; // <- UiManager 내 element 이름
        std::string currentImage;
        int posX = 0, posY = 0;
        int width = 0, height = 0;
        float rotation = 0.0f;
        float currentRotation = 0.0f;
        float currentAlpha = 1.0f;
        float currentScale = 1.0f;
        int repeatCount = 0;
        bool visibleOverrideValid = false;
        bool visibleOverrideValue = false;

        std::queue<std::shared_ptr<AnimationStep>> steps;
        std::queue<std::shared_ptr<AnimationStep>> originalSteps;
        std::shared_ptr<AnimationStep> currentStep = nullptr;
    };
    