# vcpkg 설치 및 설정 가이드

SDL2 게임 엔진 빌드에 필요한 다음 라이브러리들은 vcpkg로 설치할 수 있습니다:
- jsoncpp
- nlohmann-json
- sol2
- lua

이 라이브러리들은 vcpkg를 통해 설치할 수 있습니다.

## vcpkg 설치 방법

### 1. vcpkg 다운로드 및 설치

```powershell
# 원하는 위치로 이동 (예: C:\)
cd C:\

# vcpkg 클론
git clone https://github.com/Microsoft/vcpkg.git

# vcpkg 디렉토리로 이동
cd vcpkg

# vcpkg 부트스트랩 (빌드)
.\bootstrap-vcpkg.bat
```

### 2. 필요한 라이브러리 설치

```powershell
# MinGW 64비트 정적 라이브러리로 설치
.\vcpkg install jsoncpp:x64-mingw-static
.\vcpkg install nlohmann-json:x64-mingw-static
.\vcpkg install sol2:x64-mingw-static
.\vcpkg install lua:x64-mingw-static
```

또는 한 번에 설치:
```powershell
.\vcpkg install jsoncpp:x64-mingw-static nlohmann-json:x64-mingw-static sol2:x64-mingw-static lua:x64-mingw-static
```

### 3. 빌드 스크립트에 vcpkg 경로 설정

**build_windows.bat 파일을 열어서:**
```batch
set VCPKG_ROOT=C:\vcpkg
```

**build_windows.ps1 파일을 열어서:**
```powershell
$VcpkgRoot = "C:\vcpkg"
```

(실제 vcpkg 설치 경로로 변경)

### 4. 빌드 실행

```cmd
cd sdl2_engine
build_windows.bat
```

## 대안: 헤더 전용 라이브러리 직접 포함

일부 라이브러리는 헤더 전용이므로 직접 포함할 수 있습니다:
- nlohmann-json: https://github.com/nlohmann/json
- sol2: https://github.com/ThePhD/sol2

하지만 jsoncpp와 lua는 빌드가 필요하므로 vcpkg 사용을 권장합니다.

