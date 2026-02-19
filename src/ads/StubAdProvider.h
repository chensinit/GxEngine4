#pragma once

#include <functional>

/** 광고 제공. 현재 플랫폼: no-op / 리워드만 callback(true). Android에서 이 클래스 교체 또는 수정. */
class StubAdProvider {
public:
    void setBannerVisible(bool visible);
    /** 전면 광고 표시. onClosed: 광고가 닫힌 뒤 호출 (스텁에서는 즉시 호출). */
    void showInterstitial(std::function<void()> onClosed = nullptr);
    void showRewardedVideo(std::function<void(bool success)> callback);
};
