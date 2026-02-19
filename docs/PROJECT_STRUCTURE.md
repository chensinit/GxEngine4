# SDL2 Game Engine - 프로젝트 구조 문서

## 📁 프로젝트 개요

SDL2 기반 게임 엔진 프로젝트입니다. Windows(MinGW), macOS, Linux(Ubuntu)를 지원합니다.
Lua 스크립팅, UI 시스템, 애니메이션 시스템을 포함하며, **내부 렌더링 해상도와 창 크기를 분리**해 다양한 화면에 대응합니다.

---

## ⚙️ 설정 (setting.json)

**위치**: 프로젝트 루트 `setting.json`

엔진 시작 시 로드되며, 다음 항목을 지정할 수 있습니다.

| 키 | 설명 | 기본값 |
|----|------|--------|
| `width` | 내부 렌더링 해상도 (가로) | 800 |
| `height` | 내부 렌더링 해상도 (세로) | 600 |
| `window_width` | 실제 창 크기 (가로). 생략 시 `width`와 동일 | width |
| `window_height` | 실제 창 크기 (세로). 생략 시 `height`와 동일 | height |
| `resource_file` | 리소스 매니페스트 JSON 파일명 | "resource.json" |

**동작**: 게임은 항상 `width x height` 좌표계로 렌더링되고, `SDL_RenderSetLogicalSize`로 실제 창(`window_width x window_height`)에 맞춰 자동 스케일링됩니다. 비율이 다르면 남는 영역은 검은색(레터박스)으로 채워집니다. 안드로이드 등 다양한 해상도에서 동일한 레이아웃을 유지할 수 있습니다.

**예시**:
```json
{
  "width": 600,
  "height": 1000,
  "window_width": 600,
  "window_height": 1000,
  "resource_file": "resource_banana.json"
}
```

---

## 🗂️ 디렉토리 구조

```
sdl2_engine/
├── main.cpp, main.h              # 진입점
├── CMakeLists.txt                 # 빌드 설정
├── build.sh                       # macOS/Linux 빌드 스크립트
├── build.ps1                      # Windows PowerShell 빌드 스크립트 (선택)
├── setting.json                  # 엔진 설정 (해상도, 창 크기, 리소스 파일)
├── resource_*.json                # 리소스 매니페스트 (setting.json에서 지정)
├── resource_example/              # 예제 리소스 (이미지, 씬 JSON, Lua 스크립트)
├── resouece_banana/               # 게임별 리소스 폴더 (이름 예시)
│   ├── *.png                      # 이미지 파일들
│   ├── scene_*.json               # 씬 정의 파일
│   └── scene_*_code.lua           # 씬별 Lua 스크립트
├── data/                          # 폰트 등 데이터 파일
│   └── Jalnan2.otf
└── src/                           # 소스 코드
    ├── scene.h, scene.cpp         # 씬 관리 (게임 오케스트레이터)
    ├── animation/                  # 애니메이션 시스템
    │   ├── Animator.h/cpp         # 애니메이션 체이닝
    │   └── AnimationManager.h/cpp # 애니메이션 관리
    ├── rendering/                  # 렌더링 시스템
    │   ├── TextRenderer.h/cpp     # 텍스트 렌더링 (SDL_ttf)
    │   └── ImageRenderer.h/cpp    # 오프스크린 이미지/도형 그리기 (픽셀, 도형, 텍스트)
    ├── resource/                   # 리소스 관리
    │   └── resourceManager.h/cpp  # 텍스처, 씬 JSON, 텍스트 로딩
    ├── scripting/                  # 스크립팅 시스템
    │   ├── scriptManager.h/cpp    # Lua 바인딩 관리
    │   └── luaScriptExecutor.h/cpp # Lua 실행 엔진 (sol2)
    ├── data/                       # 데이터 관리
    │   └── userDataManager.h/cpp  # 사용자 데이터 저장/로드
    ├── ui/                         # 기본 UI 시스템
    │   └── uiManager.h/cpp        # UIElement 관리
    └── widgets/                    # 위젯 시스템 (복잡한 UI 컴포넌트)
        ├── UIWidget.h/cpp         # 위젯 베이스 클래스
        ├── WidgetManager.h/cpp    # 위젯 관리
        ├── basic/                 # 기본 + 입력
        │   ├── ButtonWidget.h/cpp
        │   ├── TextWidget.h/cpp
        │   ├── BackgroundWidget.h/cpp
        │   ├── BackgroundTextWidget.h/cpp
        │   └── EditTextWidget.h/cpp
        ├── list/                  # 리스트/컬렉션
        │   ├── TextListWidget.h/cpp
        │   ├── ChatListWidget.h/cpp
        │   ├── MultiTypeListWidget.h/cpp
        │   ├── VerticalGridWidget.h/cpp
        │   └── SectionGridWidget.h/cpp
        ├── dialog/                # 다이얼로그/알림
        │   ├── StandardDialogWidget.h/cpp
        │   ├── CustomDialogWidget.h/cpp
        │   └── ToastWidget.h/cpp
        └── rpg/                   # RPG/게임 도메인
            └── TilemapWidget.h/cpp
```

---

## 🏗️ 핵심 아키텍처

### 1. Scene (게임 오케스트레이터)
**위치**: `src/scene.h/cpp`

**역할**: 게임의 모든 시스템을 통합하고 조율하는 메인 클래스

**구성 요소**:
- `UiManager` - 기본 UI 요소 관리
- `WidgetManager` - 복잡한 위젯 관리
- `ScriptManager` - Lua 스크립트 실행
- `AnimationManager` - 애니메이션 관리
- `TextRenderer` - 텍스트 렌더링
- `ResourceManager*` - 리소스 접근

**주요 메서드**:
- `loadScene(sceneName)` - JSON에서 씬 로드, UI/위젯 초기화, Lua 스크립트 실행
- `requestSceneChange(sceneName)` - 씬 전환 요청 (다음 프레임에 처리)
- `processPendingSceneChange()` - 대기 중인 씬 전환 처리 (내부)
- `keyPressed(events)` - 입력 이벤트 처리 (위젯 우선, 일반 UI 후처리)
- `update(deltaTime)` - 위젯/애니메이션 업데이트, Lua update 호출, 씬 전환 처리
- `render()` - 모든 UIElement 렌더링

**씬 로딩 흐름**:
1. `ResourceManager`에서 씬 JSON 로드
2. 기존 씬 정리: `uiManager.clear()`, `widgetManager.clear()`, `scriptManager.reset()`
3. UI 요소 순회:
   - `type == "image"` → `UiManager::loadUIFromJson()`
   - `type == "button"`, `"text"`, `"tilemap"`, `"edittext"`, `"textlist"`, `"toast"` 등 → `WidgetManager::loadWidgetFromJson()`
4. Lua 스크립트 로드 및 `init()` 호출
5. 매니저 바인딩 재설정 (`setUiManager`, `setWidgetManager`, `setAnimationManager`)

**씬 전환 (지연 로딩)**:
- `Scene.showScene()` 호출 시 즉시 로드하지 않고 `pendingSceneName`에 저장
- 다음 프레임의 `update()` 시작 시 `processPendingSceneChange()` 호출하여 로드
- 이벤트 처리 중 씬 언로드로 인한 크래시 방지

---

### 2. UI 시스템

#### 2.1 UiManager (기본 UI)
**위치**: `src/ui/uiManager.h/cpp`

**역할**: 기본 UI 요소(`UIElement`) 관리

**UIElement 구조**:
```cpp
struct UIElement {
    std::string name;
    SDL_Texture* texture;
    std::string textureId;
    SDL_Rect rect;
    float scale, rotation, alpha;
    bool visible, clickable;
    bool useNinePatch, useThreePatch;  // changeTexture 시에도 패치 유지
    bool alwaysOnTop;                   // true면 렌더 2패스에서 최상위
    std::string parentName;              // 부모 이름 (다단계 지원)
};
```

**주요 기능**:
- `addUI(element)` - UI 요소 추가
- `findElementByName(name)` - 이름으로 찾기
- `findElementByPosition(x, y)` - 위치로 찾기
- `changeTexture(name, textureName)` - 텍스처 변경
- `move/moveTo/resize/setRect` - 위치/크기 조작
- `setAlpha/setVisible/setAlwaysOnTop/setRotate/setScale` - 변환 속성 설정
- `setAlwaysOnTop(name, onTop)` - 항상 최상위 렌더 여부 (토스트·모달 등)
- `setParent(childName, parentName)` - 부모-자식 관계 설정 (다단계 지원, 순환 참조 방지)
- `removeParent(childName)` - 부모 제거
- `getWorldPosition(name, x, y)` - 월드 위치 계산 (부모 위치 누적)
- `getWorldRotation(name)` - 월드 회전 계산 (부모 회전 누적)
- `getScaledRect(name, rect)` - 스케일 적용된 렌더링 rect 계산 (부모 스케일/회전 고려)
- `loadUIFromJson(json, renderer, textRenderer)` - JSON에서 로드

**JSON 형식**:
```json
{
    "name": "element_name",
    "type": "image" | "text",
    "loc": [x1, y1, x2, y2],
    "image": "texture_name",  // image 타입
    "text": "Hello",           // text 타입
    "textSize": 15,
    "textColor": [255, 255, 255],
    "scale": 1.0,
    "rotate": 0.0,
    "alpha": 1.0,
    "visible": true,
    "clickable": true
}
```

#### 2.2 Widget 시스템
**위치**: `src/widgets/`

**폴더 구조** (용도별 분류):
- `basic/` - 기본 블록: Button, Text, Background, BackgroundText, EditText
- `list/` - 리스트/컬렉션: TextList, ChatList, MultiTypeList, VerticalGrid, SectionGrid
- `dialog/` - 다이얼로그/알림: StandardDialog, CustomDialog, Toast
- `rpg/` - RPG/게임 도메인: Tilemap

**설계 철학**: 
- `UIWidget`는 `UIElement`를 소유하고 제어
- 위젯은 상태에 따라 `UIElement`의 속성(텍스처, 위치 등)을 변경
- 예: 버튼은 눌림 상태에 따라 이미지를 변경

**UIWidget (베이스 클래스)**:
```cpp
class UIWidget {
protected:
    std::string name;
    UiManager* uiManager;      // UIElement 제어용
    ResourceManager* resourceManager;
public:
    virtual void update(float deltaTime);
    virtual void handleEvent(const SDL_Event& event);
    virtual void render(SDL_Renderer* renderer);
    
    // 포커스 관리 (EditText 등에서 사용)
    virtual void setFocus(bool focus) {}
    virtual bool canReceiveFocus() const { return false; }
    virtual bool isPointInside(int x, int y) const { return false; }
};
```

**WidgetManager**:
- `std::unordered_map<std::string, std::unique_ptr<UIWidget>>`로 위젯 관리
- `loadWidgetFromJson(json)` - JSON에서 위젯 생성
- `update/handleEvents/render` - 위젯들 일괄 처리
- **포커스 관리**: `focusedWidget`로 현재 입력 포커스를 가진 위젯 추적
  - 클릭 시 포커스 변경 (`setFocusedWidget()`)
  - `SDL_TextInput` 시작/중지 자동 관리
  - 포커스된 위젯이 `SDL_TEXTINPUT` 이벤트를 우선 처리
- **클리핑 관리**: `render()`에서 각 위젯 렌더링 전에 위젯 영역으로 클리핑 설정
  - 위젯의 월드 위치와 크기를 계산하여 `SDL_RenderSetClipRect()` 설정
  - 위젯 경계를 넘어서는 렌더링 자동 제한 (예: TextListWidget 스크롤)

**구현된 위젯들**:

1. **ButtonWidget**: 클릭 가능한 버튼
   - 상태: `NORMAL`, `PRESSED`, `DISABLED`
   - 각 상태별 이미지: `normalImage`, `pressedImage`, `disabledImage`
   - 텍스트 오버레이 지원 (`text`, `textSize`, `textColor`)
   - `setOnClick(callback)` - 클릭 콜백 설정
   - 마우스 이벤트 처리로 상태 변경 및 콜백 호출

2. **TextWidget**: 정적 텍스트 표시
   - 텍스트 자동 래핑 및 높이 제한 지원
   - 텍스트 변경 시 텍스처 자동 재생성
   - 개별 텍스처 ID 자동 생성 (캐싱 없음)

3. **BackgroundWidget**: 색상/이미지 배경 표시
   - 색상 배경 (`bgColor`) 또는 이미지 배경 (`image`)
   - `imageMode`: stretch, tile, fit, scroll_h, scroll_v
   - scroll 모드: child 이미지 + AnimationManager, ping-pong(왕복) 스크롤
   - `scrollCycleTime` / `scrollDuration`: 한 방향 이동 소요 시간(초)

4. **TextListWidget**: 텍스트 리스트 표시
   - 여러 항목을 리스트로 표시
   - `addItem()`, `removeItem()`, `clearItems()` 지원
   - **스크롤 지원**: 마우스 드래그로 스크롤 가능
   - 위젯 영역을 넘어서는 텍스트는 자동으로 클리핑됨
   - 스크롤 범위는 항목 수와 위젯 높이에 따라 자동 계산

5. **TilemapWidget**: 타일맵 렌더링
   - 맵 파일 또는 직접 지정 방식 지원
   - 카메라 오프셋 지원 (`setOffset()`)
   - 위젯 위치와 크기를 고려한 렌더링
   - 화면에 보이지 않는 타일은 자동으로 스킵

6. **EditTextWidget**: 텍스트 입력
   - 포커스 기반 입력 처리 (`canReceiveFocus()`)
   - `SDL_TEXTINPUT` 이벤트 처리
   - 커서 표시 및 편집 기능 (백스페이스, 화살표 키 등)
   - Placeholder 텍스트 지원
   - 배경색, 테두리 색상 커스터마이징 가능

7. **ChatListWidget**: 채팅 리스트 표시
   - 채팅 앱처럼 왼쪽/오른쪽 정렬 메시지 표시
   - 각 메시지는 아이콘과 텍스트로 구성
   - `addMessage()`, `clearMessages()` 지원
   - **스크롤 지원**: 마우스 드래그로 스크롤 가능
   - 위젯 영역을 넘어서는 메시지는 자동으로 클리핑됨
   - 내부적으로 각 메시지의 아이콘과 텍스트를 개별 UIElement로 관리

8. **ToastWidget**: 토스트 메시지 표시
   - 일시적 메시지 (예: "다음 업데이트때 지원") 표시
   - Lua: `widget.showToast("toast_message", "텍스트", 3000)`, `widget.hideToast("toast_message")`
   - 텍스트 UI는 배경의 자식으로 부모 alpha 상속 (페이드 인/아웃 함께 적용)
   - 배경·텍스트 요소에 `alwaysOnTop` 자동 설정 → 코드에서 나중에 추가되는 UI보다 항상 위에 렌더
   - JSON `loc`를 `[0, y1, screen_width, y2]`로 주면 화면 가로 전체 사용 권장

9. **SectionGridWidget**: 헤더 + 섹션 그리드 표시
   - `addOverlay(overlayId, imageName, height)`, `addHeader(imageName)`, `addCard(imageName, overlayId)`
   - 오버레이 공용 정의, 카드별 overlayId 지정
   - Lazy loading: 뷰포트에 보이는 항목만 UI·텍스처 로드
   - `clear()` 시 스크롤 위치 0으로 리셋
   - `layout()` - items 추가 후 한 번만 호출 (성능 최적화)
   - 마우스 드래그 스크롤 지원
   - Lua: `addSectionOverlay`, `addSectionHeader`, `addSectionCard`, `clearSectionGrid`, `setSectionGridHeaderScale`, `layoutSectionGrid`

**JSON 형식**:
```json
{
    "name": "button_name",
    "type": "button",
    "loc": [x1, y1, x2, y2],
    "normalImage": "normal_texture",
    "pressedImage": "pressed_texture",
    "disabledImage": "disabled_texture",  // 선택
    "scale": 1.0,
    "rotate": 0.0,
    "alpha": 1.0,
    "visible": true
}
```

---

### 3. 애니메이션 시스템

#### Animator
**위치**: `src/animation/Animator.h/cpp`

**역할**: 체이닝 방식의 애니메이션 빌더

**사용법**:
```cpp
auto anim = std::make_shared<Animator>("ui_element_name");
anim->rotate(0.0f, 360.0f, 1000)  // 0도→360도, 1초
    ->move(100, 50, 500)          // 이동, 0.5초
    ->resize(200, 200, 300)       // 크기 변경, 0.3초
    ->repeat(-1)                  // 무한 반복
    ->callback([]() { ... });     // 완료 콜백
```

**지원 애니메이션**:
- `changeTexture(imageName)` - 텍스처 변경
- `rotate(startDeg, endDeg, durationMs)` - 회전
- `moveTo(startX, startY, endX, endY, durationMs)` - 절대 이동
- `alpha(startAlpha, endAlpha, durationMs)` - 투명도 변경
- `scale(startScale, endScale, durationMs)` - 스케일 변경
- `delay(durationMs)` - 지연
- `repeat_anim(count)` - 반복 (-1 = 무한)
- `callback(function)` - 완료 콜백

#### AnimationManager
**위치**: `src/animation/AnimationManager.h/cpp`

**역할**: 여러 `Animator` 인스턴스 관리 및 UI 반영

**사용법**:
```cpp
animationManager.add("element_name", animator);
// 매 프레임 update() 호출 시 자동으로 UI에 반영
```

**동작**:
1. `Animator::update(deltaTime)` 호출
2. 애니메이션 결과를 `UiManager`에 반영:
   - 텍스처 변경: `uiManager->changeTexture()`
   - 위치 변경: `uiManager->moveTo()`
   - 스케일 변경: `uiManager->setScale()`
   - 회전 변경: `uiManager->setRotate()`
   - 투명도 변경: `uiManager->setAlpha()`
3. 완료된 애니메이션 자동 제거
4. 부모-자식 관계를 고려하여 월드 좌표/스케일/회전 계산

---

### 4. 스크립팅 시스템

#### ScriptManager
**위치**: `src/scripting/scriptManager.h/cpp`

**역할**: Lua 스크립트와 C++ 바인딩

**Lua API**:

**UI 조작** (`ui` 테이블):
```lua
ui.changeImage("element_name", "new_texture")
ui.move("element_name", dx, dy)
ui.moveto("element_name", x, y)
ui.resize("element_name", w, h)
ui.setRect("element_name", x, y, w, h)
ui.setAlpha("element_name", alpha)
ui.setVisible("element_name", true/false)
ui.setRotate("element_name", angle)
ui.setScale("element_name", scale)
-- Getter들도 동일한 패턴
```

**위젯 조작** (`widget` 테이블):
```lua
-- 위젯의 UI element ID 조회 (ui/애니메이션 API에 전달용)
local id = widget.getUIElementId("widget_name")  -- 위젯 이름 → UiManager element ID

widget.setButtonCallback("button_name", function()
    print("Button clicked!")
end)
widget.setButtonEnabled("button_name", true/false)
widget.showToast("toast_message", "표시할 텍스트", 3000)  -- durationMs (선택, 기본 3000)
widget.hideToast("toast_message")
```

**애니메이션** (`animation` 테이블):
```lua
local anim = Animator("element_name")  -- UI 요소 이름으로 Animator 생성
anim:rotate(0, 360, 1000)
    :scale(1.0, 1.5, 1000)
    :alpha(1.0, 0.5, 500)
    :repeat_anim(-1)
animation.add("element_name", anim:get())  -- 첫 번째 파라미터는 UI 요소 이름 (애니메이션 이름 아님!)
```

**씬 전환** (`Scene` 테이블):
```lua
Scene.showScene("scene_name")
```

**이벤트 핸들러**:
```lua
function init()
    -- 씬 로드 시 한 번 호출
end

function update()
    -- 매 프레임 호출
end

function keyPressed(elementName, type, keycode, x, y)
    -- 입력 이벤트 처리
    -- type: "key_down", "key_up", "mouse_down", "mouse_up", "mouse_click"
end
```

---

### 5. 렌더링 시스템

#### ImageRenderer
**위치**: `src/rendering/ImageRenderer.h/cpp`

**역할**: 오프스크린 버퍼에 픽셀/도형/텍스트/이미지 그리기. SDL_Surface 기반으로 그린 뒤 SDL_Texture로 변환해 출력용으로 사용.

**주요 기능**: `drawPixel`, `drawColor`, `drawText`, `drawRect`, `drawCircle`, `drawImage`, `getTexture`, `clear`

#### UiManager 렌더링
**위치**: `src/ui/uiManager.cpp`

**역할**: 모든 UIElement 렌더링 및 클리핑 관리

**주요 기능**:
- `render(SDL_Renderer* renderer)` - 2패스 렌더: 1패스 일반 요소, 2패스 `alwaysOnTop` 요소 (토스트 등 최상위)
- `renderElement(renderer, element)` - 단일 요소 그리기 (내부 공용)
- 부모-자식 관계에 따른 월드 위치 계산 (다단계 지원)
- **alpha 상속**: 자식의 최종 alpha = 자식 alpha × 부모 체인 alpha (렌더 시 적용)
- 자식 UIElement는 부모 영역으로 조건부 클리핑 (`SDL_RenderSetClipRect`)
  - 기본적으로 클리핑 활성화
  - 부모나 본인이 스케일/회전 중일 때는 클리핑 비활성화 (잘림 방지)
- 스케일, 알파, 회전 변환 적용

#### WidgetManager 렌더링
**위치**: `src/widgets/WidgetManager.cpp`

**역할**: 위젯별 특수 렌더링 및 클리핑 관리

**주요 기능**:
- `render(SDL_Renderer* renderer)` - 위젯별 직접 렌더링 (TextListWidget, TilemapWidget 등)
- 각 위젯 렌더링 전에 위젯 영역으로 클리핑 설정
- 위젯 경계를 넘어서는 렌더링 자동 제한

---

### 6. 리소스 관리

#### ResourceManager
**위치**: `src/resource/resourceManager.h/cpp`

**역할**: 텍스처, 씬 JSON, 텍스트 파일 로딩 및 캐싱

**리소스 파일** (파일명은 `setting.json`의 `resource_file`로 지정):
```json
{
    "initial_scene": "scene_menu",
    "resource_folder": "resource_example",
    "resources": [
        {"name": "texture_name", "type": "image", "path": "image.png"},
        {"name": "scene_name", "type": "scene", "path": "scene.json"},
        {"name": "script_name", "type": "text", "path": "scene_code.lua"},
        {"name": "map_name", "type": "json", "path": "map.json"}
    ]
}
```
- `initial_scene`: 최초 로드할 씬 이름
- `resource_folder`: 리소스 경로 접두사 (선택). 있으면 모든 `path` 앞에 붙음
- `resources`: `name`, `type`(image | scene | json | text), `path`

**주요 메서드**:
- `loadResources(resourceFilePath)` - 리소스 매니페스트 로드
- `getTexture(imageName)` - 텍스처 가져오기 (캐시)
- `getSceneJson(sceneName)` - 씬 JSON 가져오기
- `getMapJson(mapName)` - 맵 JSON 가져오기 (lazy loading)
- `getText(name)` - 텍스트 파일 내용 가져오기
- `getInitialScene()` - 초기 씬 이름 반환

**텍스처 등록/해제 (동적 텍스처용)**:
- `registerTexture(textureName, texture)` - 이름 지정 등록 (⚠️ JSON 로드용으로만 사용 권장)
  - 이미 등록된 이름이면 기존 ID 반환 및 참조 카운트 증가
- `registerTexture(texture)` - 자동 ID 생성 등록 (권장)
  - `"dynamic_N"` 형식의 고유 ID 자동 생성
- `unregisterTexture(textureId)` - 텍스처 해제
  - 참조 카운트 감소, 0이 되면 텍스처 삭제

---

## 🔄 실행 흐름

### 초기화
1. `main.cpp`에서 `setting.json` 로드 (내부 해상도 `width`/`height`, 창 크기 `window_width`/`window_height`, `resource_file`)
2. SDL 초기화, 창 생성(`window_width` x `window_height`), 렌더러 생성
3. `SDL_RenderSetLogicalSize(renderer, width, height)`로 논리 해상도 설정 (스케일링·레터박스 자동)
4. `ResourceManager` 생성 및 `resource_file`에 지정된 리소스 매니페스트 로드
5. `Scene` 생성 (모든 매니저 초기화)
6. `ResourceManager::getInitialScene()`으로 초기 씬 이름 획득 후 `Scene::loadScene()` 호출

### 게임 루프 (60fps)
```cpp
while (running) {
    // 1. 이벤트 수집
    events = SDL_PollEvent(...)
    
    // 2. 입력 처리
    scene.keyPressed(events)
    
    // 3. 업데이트
    scene.update(deltaTime)
    
    // 4. 렌더링
    scene.render()
    SDL_RenderPresent()
}
```

### 씬 로딩 과정
1. `Scene::loadScene(sceneName)` 호출
2. `ResourceManager`에서 씬 JSON 로드
3. UI 요소 순회:
   - `image`/`text` → `UiManager::loadUIFromJson()`
   - `button` 등 → `WidgetManager::loadWidgetFromJson()`
4. Lua 스크립트 로드 및 `init()` 호출

### 이벤트 처리 우선순위
1. 위젯 이벤트 처리 (`WidgetManager::handleEvents()`)
   - 포커스된 위젯이 `SDL_TEXTINPUT` 우선 처리
   - 일반 키 입력은 포커스된 위젯이 있으면 소비 (단축키 제외)
2. 일반 UI 요소 클릭 감지
3. Lua `keyPressed()` 호출
   - 위젯에서 소비되지 않은 이벤트만 전달

---

## 🛠️ 빌드 방법

### Windows (MinGW)
```powershell
# PowerShell
cd sdl2_engine
$env:VCPKG_ROOT = "D:\work\cpp\vcpkg"
$env:PATH = "D:\app\minGW\w64devkit\bin;$env:PATH"
cmake --build build --config Release

# 또는 배치 파일
.\build_windows.bat
```

### macOS / Linux
```bash
cd sdl2_engine
./build.sh
```
(실행 파일: `build/main`)

**의존성**:
- SDL2, SDL2_image, SDL2_ttf
- vcpkg: jsoncpp, nlohmann-json, sol2, lua

---

## 📝 주요 설계 원칙

1. **Composition over Inheritance**: 위젯이 UIElement를 소유하고 제어
2. **Separation of Concerns**: 각 매니저가 독립적인 책임
3. **JSON-Driven**: 씬과 UI는 JSON으로 정의
4. **Script-Driven Logic**: 게임 로직은 Lua로 작성
5. **Widget Extensibility**: `src/widgets/`에 새 위젯 추가 용이

---

## 🔮 확장 가이드

### 새 위젯 추가하기
1. `src/widgets/` 하위에 용도에 맞는 폴더 선택 후 클래스 생성
   - 기본 UI → `basic/` (예: `basic/ListViewWidget.h/cpp`)
   - 리스트/컬렉션 → `list/`
   - 다이얼로그/알림 → `dialog/`
   - 게임 도메인 → `rpg/`
2. `UIWidget` 상속
3. `WidgetManager::loadWidgetFromJson()`에 타입 분기 추가
4. JSON에서 `"type": "listview"` 사용 가능

### 새 애니메이션 타입 추가
1. `Animator::AnimationStep::Type`에 새 타입 추가
2. `Animator`에 체이닝 메서드 추가
3. `Animator::update()`에서 새 타입 처리

### Lua API 확장
1. `ScriptManager::setCommonApi()` 또는 각 매니저 바인딩 메서드에 함수 추가
2. `sol::table`에 함수 등록

---

## 📌 참고사항

- **Delta Time**: `main.cpp`에서 첫 프레임의 큰 deltaTime을 제한 (100ms 이상 → 16.67ms)
- **Include 경로**: `src/` 루트가 include 디렉토리로 설정됨 (`CMAKE_SOURCE_DIR`)
- **위젯 우선순위**: 위젯 이벤트가 일반 UI 이벤트보다 먼저 처리됨
- **애니메이션**: `AnimationManager`는 `UiManager`를 참조하여 UI에 반영
- **해상도**: 내부 렌더링은 `setting.json`의 `width`/`height`, 창 크기는 `window_width`/`window_height`. 논리 해상도로 스케일링되며 남는 영역은 검은색

---

## 🐛 알려진 이슈

### MultiTypeListWidget의 클리핑(Scissor) 제한

**현재 상황:**
- `MultiTypeListWidget`에서 화면 밖으로 나간 항목은 `updateVisibleItems()`에서 정상적으로 제거됨
- 하지만 텍스트 UIElement는 부모(`backgroundElement`)에만 클리핑되고, **리스트 뷰포트 전체 기준 scissor는 적용되지 않음**

**원인:**
- `WidgetManager::render()`에서 위젯 영역으로 클리핑을 설정하지만, `MultiTypeListWidget::render()`는 비어 있어서 아무것도 렌더링하지 않음
- 실제 렌더링은 `UiManager::render()`에서 수행되는데, 이는 별도로 호출되어 `WidgetManager`에서 설정한 클리핑이 적용되지 않음
- `UiManager::render()`는 부모-자식 관계 기반으로만 클리핑을 처리하므로, 리스트 컨테이너(`uiElementId`) 기준 클리핑은 자식의 직접 부모까지만 적용됨

**영향:**
- 치명적인 문제는 아님 (항목 제거 로직은 정상 작동)
- 다만 화면 밖으로 나간 텍스트가 잠깐 보일 수 있는 시각적 문제 가능성
- 성능에는 큰 영향 없음 (항목 자체는 제거되므로)

**향후 개선 방향:**
- `UiManager::render()`에서 부모 체인 전체의 rect를 교집합하여 클리핑하도록 개선
- 또는 `WidgetManager::render()`에서 `UiManager::render()`를 호출하여 위젯별 클리핑 범위 내에서만 렌더링하도록 구조 변경

---

**마지막 업데이트**: 2025-02-14
**프로젝트 상태**: 활발히 개발 중

---

## 최근 업데이트 (2025-02-14)

### UI / 렌더링
- **3패치**: 가로 3등분(좌/중/우, 중간만 늘림) 지원. image/button/backgroundtext 등 `useThreePatch` 옵션.
- **9패치·3패치 유지**: `changeTexture` 시 해당 요소가 useNinePatch/useThreePatch면 새 텍스처도 같은 방식으로 생성. `createTextureForImage` 헬퍼로 로드/변경 경로 통합.
- **alwaysOnTop + 2패스 렌더**: UIElement에 `alwaysOnTop` 플래그, `setAlwaysOnTop(name, onTop)`. render()는 1패스 일반 → 2패스 alwaysOnTop 순. 토스트가 항상 최상위에 그려짐.

### 위젯
- **BannerListWidget**: `itemMargin` (항목 간 세로 간격) JSON 지원.
- **ToastWidget**: 생성 시 배경·텍스트에 alwaysOnTop 설정. loc를 화면 폭에 맞추면 가로 전체 사용 가능.

---

## 최근 업데이트 (2025-02)

### 주요 추가 기능
- **위젯 폴더 구조 개편**: 용도별 분류 (basic/, list/, dialog/, rpg/)
- **SectionGridWidget**: 헤더 + 섹션 그리드 위젯
  - `addSectionOverlay`, `addHeader(imageName)`, `addCard(imageName, overlayId)`
  - 오버레이 공용 정의, lazy loading (뷰포트 내 항목만 UI·텍스처 로드)
  - `clear()` 시 스크롤 위치 리셋
- **widget.getUIElementId(name)**: 위젯 이름 → UiManager element ID 조회. 위젯에 `ui.setAlpha`, `ui.setVisible`, `animation.add` 적용 시 사용
- **ui.setVisible(name, visible)**: UI 요소 표시/숨김
- **설정 (setting.json)**: 내부 렌더링 해상도(`width`/`height`)와 창 크기(`window_width`/`window_height`) 분리, `resource_file`로 리소스 매니페스트 지정. 논리 해상도 스케일링 및 레터박스 자동 적용
- **EditTextWidget**: 텍스트 입력 위젯 구현
  - 포커스 기반 입력 처리
  - 커서 표시 및 편집 기능
  - Placeholder 텍스트 지원
- **TextListWidget**: 텍스트 리스트 위젯 구현
  - 마우스 드래그 스크롤 지원
  - 위젯 영역 클리핑 자동 적용
- **ChatListWidget**: 채팅 리스트 위젯 구현
  - 왼쪽/오른쪽 정렬 메시지 표시
  - 아이콘과 텍스트를 개별 UIElement로 관리
  - 스크롤 및 클리핑 지원
  - Lua API에서 메시지 추가 시 자동 스크롤 기능
- **포커스 관리 시스템**: WidgetManager에서 중앙 집중식 포커스 관리
- **씬 전환 개선**: 지연 로딩 방식으로 크래시 방지
- **텍스처 ID 관리 개선**: 자동 ID 생성, 이름 지정은 JSON 로드용으로만 사용
- **타일맵 위젯 개선**: 위젯 위치와 크기를 고려한 렌더링
- **렌더링 시스템 개선**: 
  - UiManager에서 모든 UIElement 렌더링 및 부모-자식 클리핑 처리
  - WidgetManager에서 위젯별 클리핑 자동 적용

