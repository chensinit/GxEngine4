#include "StubAdProvider.h"

void StubAdProvider::setBannerVisible(bool /*visible*/) {
    // no-op; Android 빌드에서 배너 표시/숨김 구현
}

void StubAdProvider::showInterstitial(std::function<void()> onClosed) {
    // no-op; Android: InterstitialAd.fullScreenContentCallback 에서
    // FullScreenContentCallback.onAdDismissedFullScreenContent() 호출 시 onClosed() 호출 가능
    if (onClosed) onClosed();
}

void StubAdProvider::showRewardedVideo(std::function<void(bool success)> callback) {
    if (callback) {
        callback(true);  // 현재 플랫폼은 항상 성공으로 처리
    }
}
