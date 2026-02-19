# macOS 빌드 가이드

SDL2 게임 엔진을 macOS에서 빌드하는 방법을 안내합니다.

## 사전 요구사항

### 1. Homebrew 설치

Homebrew가 설치되어 있지 않다면 다음 명령어로 설치하세요:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 2. 필요한 라이브러리 설치

다음 명령어로 필요한 라이브러리들을 설치하세요:

```bash
brew install sdl2 sdl2_image sdl2_ttf jsoncpp nlohmann-json lua cmake
```

또는 하나씩 설치:

```bash
brew install sdl2          # SDL2 코어 라이브러리
brew install sdl2_image    # SDL2 이미지 로딩
brew install sdl2_ttf      # SDL2 트루타입 폰트
brew install jsoncpp       # JSON 파싱 라이브러리
brew install nlohmann-json # 헤더 전용 JSON 라이브러리
brew install lua           # Lua 스크립팅 엔진
brew install cmake         # 빌드 시스템
```

### 3. sol2 설치 (선택사항)

sol2는 C++에서 Lua를 사용하기 위한 헤더 전용 라이브러리입니다. vcpkg를 사용하거나 직접 포함할 수 있습니다.

**옵션 A: vcpkg 사용 (권장)**

```bash
# vcpkg 설치
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

# sol2 설치 (아키텍처에 맞게)
# Apple Silicon (M1/M2/M3)
./vcpkg install sol2:arm64-osx

# Intel Mac
./vcpkg install sol2:x64-osx
```

그 다음 `build.sh`에서 vcpkg 경로를 설정하세요:

```bash
export VCPKG_ROOT=$HOME/vcpkg
./build.sh
```

**옵션 B: 직접 포함**

sol2는 헤더 전용 라이브러리이므로 직접 다운로드하여 프로젝트에 포함할 수 있습니다.

## 빌드 방법

### 방법 1: 빌드 스크립트 사용 (권장)

```bash
cd sdl2_engine
./build.sh
```

빌드 타입을 지정하려면:

```bash
./build.sh Release  # Release 빌드 (기본값)
./build.sh Debug    # Debug 빌드
```

### 방법 2: 수동 빌드

```bash
cd sdl2_engine
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

vcpkg를 사용하는 경우:

```bash
cd sdl2_engine
mkdir -p build
cd build
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## 빌드 결과

빌드가 성공하면 실행 파일이 생성됩니다:

```bash
./build/main
```

실행 시 프로젝트 루트의 `setting.json`을 읽어 해상도·창 크기·리소스 파일을 적용합니다. 자세한 내용은 `USER_GUIDE.md`의 "설정 (setting.json)"을 참고하세요.

**주의:**
- **항상 프로젝트 루트에서 실행**합니다. (`./build/main` 실행 시에도 CWD는 루트여야 함) 리소스는 루트 기준 경로로 로드됩니다.
- build 폴더는 실행에 사용하지 않으며, **리소스를 build 폴더로 복사하거나 수정하지 마세요.**

## 문제 해결

### SDL2를 찾을 수 없는 경우

```bash
# SDL2 설치 확인
brew list sdl2

# 설치되어 있지 않다면
brew install sdl2 sdl2_image sdl2_ttf

# CMake 캐시 삭제 후 재빌드
rm -rf build
./build.sh
```

### jsoncpp를 찾을 수 없는 경우

```bash
brew install jsoncpp
```

### sol2를 찾을 수 없는 경우

vcpkg를 사용하거나, sol2를 직접 포함하세요. sol2는 헤더 전용 라이브러리이므로 빌드가 필요 없습니다.

### lua를 찾을 수 없는 경우

```bash
brew install lua
```

### 기타 문제

CMake 캐시를 삭제하고 다시 빌드해보세요:

```bash
rm -rf build
./build.sh
```

## 아키텍처별 참고사항

- **Apple Silicon (M1/M2/M3)**: `arm64-osx` 트리플릿 사용
- **Intel Mac**: `x64-osx` 트리플릿 사용

vcpkg를 사용하는 경우 아키텍처에 맞는 트리플릿으로 패키지를 설치해야 합니다.

