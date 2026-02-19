# Windows 빌드 가이드

이 문서는 SDL2 게임 엔진을 Windows에서 빌드하는 방법을 안내합니다. macOS는 `README_MACOS.md`를 참고하세요.

## 사전 요구사항

### 1. CMake 설치
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

### 2. MinGW 설정
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

