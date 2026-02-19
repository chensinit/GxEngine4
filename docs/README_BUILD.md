# Windows 빌드 가이드

이 문서는 SDL2 게임 엔진을 Windows에서 빌드하는 방법을 안내합니다. macOS는 `README_MACOS.md`를 참고하세요.

## 사전 요구사항

### 1. 의존 라이브러리 세팅

프로젝트는 아래 라이브러리를 **땡겨와서** 사용합니다. 플랫폼별로 설치 방법이 다릅니다.

| 라이브러리 | 용도 | Windows | macOS | Linux |
|------------|------|---------|--------|--------|
| **SDL2** | 창/입력/오디오 | MinGW용 prebuilt (`../libs/` 경로) 또는 vcpkg | Homebrew | 패키지 매니저 |
| **SDL2_image** | 이미지 로드 (PNG 등) | 위와 동일 | `brew install sdl2_image` | `libsdl2-image-dev` |
| **SDL2_ttf** | 트루타입 폰트 렌더링 | 위와 동일 | `brew install sdl2_ttf` | `libsdl2-ttf-dev` |
| **jsoncpp** | JSON 파싱 (C++) | vcpkg | `brew install jsoncpp` | `libjsoncpp-dev` |
| **nlohmann_json** | JSON 파싱 (헤더 전용) | vcpkg | `brew install nlohmann-json` | `nlohmann-json3-dev` |
| **sol2** | C++ ↔ Lua 바인딩 | 프로젝트 내 `third_party/sol2` 포함 또는 vcpkg | 동일 (third_party 사용) | 동일 |
| **Lua** | 스크립트 엔진 (sol2가 사용) | vcpkg | `brew install lua` | `liblua5.4-dev` 등 |

#### Windows에서 vcpkg로 한 번에 설치

SDL2 계열은 MinGW prebuilt를 쓰는 경우 `../libs/`에 두고, 나머지(jsoncpp, nlohmann_json, sol2, lua)는 vcpkg로 넣는 구성을 많이 씁니다.

1. vcpkg 설치 (한 번만):
   ```powershell
   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
   cd C:\vcpkg
   .\bootstrap-vcpkg.bat
   ```

2. 필요한 패키지 설치 (MinGW 64비트 정적 예시):
   ```powershell
   .\vcpkg install jsoncpp:x64-mingw-static
   .\vcpkg install nlohmann-json:x64-mingw-static
   .\vcpkg install lua:x64-mingw-static
   ```
   sol2는 이 프로젝트에 `third_party/sol2`가 포함되어 있으면 vcpkg 없이 사용됩니다.

3. CMake 빌드 시 vcpkg 툴체인 지정:
   ```powershell
   $env:VCPKG_ROOT = "C:\vcpkg"
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
   ```

#### macOS에서 Homebrew로 설치

```bash
brew install cmake
brew install sdl2 sdl2_image sdl2_ttf
brew install jsoncpp nlohmann-json lua
```

sol2는 `third_party/sol2`에 포함되어 있으므로 별도 설치 없이 빌드됩니다.

#### Linux (Ubuntu/Debian 예시)

```bash
sudo apt install cmake build-essential
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev
sudo apt install libjsoncpp-dev nlohmann-json3-dev liblua5.4-dev
```

---

### 2. CMake 설치
CMake가 설치되어 있지 않다면 다음 중 하나의 방법으로 설치하세요:

1. **공식 웹사이트**: https://cmake.org/download/
   - Windows x64 Installer 다운로드 및 설치
   - 설치 시 "Add CMake to system PATH" 옵션 체크

2. **winget 사용**:
   ```powershell
   winget install Kitware.CMake
   ```

3. **Chocolatey 사용**:
   ```powershell
   choco install cmake
   ```

CMake가 설치되어 있지만 PATH에 없다면, 빌드 스크립트 상단의 `CMAKE_PATH` 또는 `$CmakePath` 변수에 직접 경로를 지정하세요.

### 3. MinGW 설정 (Windows만)
MinGW를 사용하는 경우, 빌드 스크립트가 자동으로 다음 경로를 검색합니다:
- `C:\mingw64`
- `C:\msys64\mingw64`
- `C:\MinGW`

다른 경로에 설치되어 있다면, 사용하는 빌드 스크립트(예: `build.ps1`) 상단의 MinGW 경로 변수에 경로를 지정하세요:

```powershell
$MingwRoot = "C:\your\mingw\path"
```

## 빌드 방법

### 방법 1: PowerShell 스크립트 사용 (build.ps1이 있는 경우)

```powershell
cd sdl2_engine
powershell -ExecutionPolicy Bypass -File build.ps1
```

실행 정책을 변경한 경우:
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
cd sdl2_engine
.\build.ps1
```

### 방법 2: 수동 빌드

```powershell
cd sdl2_engine
mkdir build -ErrorAction SilentlyContinue
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## vcpkg 사용하는 경우

다른 라이브러리(jsoncpp, nlohmann_json, sol2, lua)가 vcpkg로 설치되어 있다면:

**빌드 스크립트 또는 환경 변수:**
```powershell
$env:VCPKG_ROOT = "C:\vcpkg"
```

**수동 빌드 시:**
```powershell
cmake -Bbuild -S. -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
```

## 빌드 결과

빌드가 성공하면:
- 실행 파일: `build/main.exe` (또는 `build/Release/main.exe` — CMake 설정에 따름)
- Windows MinGW 빌드 시 필요한 SDL2 DLL 파일들이 자동으로 같은 폴더에 복사됩니다.

## 실행 시 설정

실행 시 프로젝트 루트의 `setting.json`을 읽어 해상도·창 크기·리소스 파일을 적용합니다. 자세한 내용은 `USER_GUIDE.md`의 "설정 (setting.json)"을 참고하세요.

**주의:**
- **항상 프로젝트 루트에서 실행**합니다. 리소스(이미지, JSON 등)는 루트 기준 경로로 로드됩니다.
- build 폴더는 실행에 사용하지 않으며, **리소스를 build 폴더로 복사하거나 수정하지 마세요.**

