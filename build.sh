#!/bin/bash

# macOS 빌드 스크립트

# vcpkg 경로 설정 (선택사항)
# vcpkg를 사용하는 경우 아래 주석을 해제하고 경로를 수정하세요
# export VCPKG_ROOT=$HOME/vcpkg

# 빌드 타입 설정 (Release 또는 Debug)
BUILD_TYPE=${1:-Release}

echo "Building for macOS..."
echo "Build type: ${BUILD_TYPE}"

# vcpkg를 사용하는 경우
if [ -n "$VCPKG_ROOT" ] && [ -d "$VCPKG_ROOT" ]; then
    echo "Using vcpkg at: $VCPKG_ROOT"
    
    # macOS 아키텍처 확인
    ARCH=$(uname -m)
    if [ "$ARCH" = "arm64" ]; then
        TRIPLET="arm64-osx"
    else
        TRIPLET="x64-osx"
    fi
    
    cmake -Bbuild -S. \
        -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
else
    echo "Using system packages (Homebrew)"
    echo "Make sure you have installed: sdl2 sdl2_image sdl2_ttf jsoncpp nlohmann-json lua"
    echo "Install with: brew install sdl2 sdl2_image sdl2_ttf jsoncpp nlohmann-json lua"
    
    cmake -Bbuild -S. \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
fi

cmake --build build