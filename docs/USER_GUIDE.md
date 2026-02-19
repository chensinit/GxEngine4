# SDL2 Game Engine - 사용자 가이드

이 문서는 게임 엔진 사용자를 위한 API 가이드입니다. 개발자용 구조 문서는 `PROJECT_STRUCTURE.md`를 참고하세요.

---

## 📋 목차

1. [설정 (setting.json)](#설정-settingjson)
2. [기본 UI 요소](#기본-ui-요소)
3. [위젯 시스템](#위젯-시스템)
4. [타일맵 시스템](#타일맵-시스템)
5. [부모-자식 관계](#부모-자식-관계)
6. [애니메이션](#애니메이션)
7. [Lua 스크립팅](#lua-스크립팅)
8. [입력 포커스 관리](#입력-포커스-관리)

---

## 설정 (setting.json)

엔진이 사용하는 전역 설정 파일입니다. 프로젝트 루트에 `setting.json`을 두면 시작 시 자동으로 로드됩니다.

**항목**:

| 키 | 설명 | 기본값 |
|----|------|--------|
| `width` | 내부 렌더링 해상도 (가로). 게임 좌표계 기준 | 800 |
| `height` | 내부 렌더링 해상도 (세로) | 600 |
| `window_width` | 실제 창 가로 크기. 생략 시 `width` 사용 | width |
| `window_height` | 실제 창 세로 크기. 생략 시 `height` 사용 | height |
| `resource_file` | 리소스 매니페스트 JSON 파일명 | "resource.json" |

**동작**: 게임은 항상 `width` x `height`로 그리며, 창 크기와 다르면 자동으로 스케일링됩니다. 비율이 다르면 남는 영역은 검은색(레터박스)으로 채워집니다.

**예시** (내부 600x1000, 창 500x800):
```json
{
  "width": 600,
  "height": 1000,
  "window_width": 500,
  "window_height": 800,
  "resource_file": "resource_banana.json"
}
```

---

## 기본 UI 요소

### 이미지 (Image)

가장 기본적인 UI 요소입니다. 이미지를 화면에 표시합니다.

**JSON 형식:**
```json
{
    "name": "background",
    "type": "image",
    "image": "texture_name",
    "loc": [0, 0, 800, 600],
    "scale": 1.0,
    "rotate": 0.0,
    "alpha": 1.0,
    "visible": true,
    "clickable": false
}
```

**필드 설명:**
- `name`: 요소의 고유 이름
- `type`: `"image"` 고정
- `image`: `resource.json`에 등록된 텍스처 이름
- `loc`: 위치와 크기 `[x1, y1, x2, y2]`
- `useNinePatch`: true면 9패치로 확대/축소 (기본값: false, 이름에 `_9patch`/`_9p` 있으면 true)
- `useThreePatch`: true면 가로 3등분(좌/중/우, 중간만 늘림) (기본값: false). useNinePatch와 둘 다 true면 9패치 우선
- `scale`: 스케일 (기본값: 1.0)
- `rotate`: 회전 각도 (도, 기본값: 0.0)
- `alpha`: 투명도 0.0~1.0 (기본값: 1.0)
- `visible`: 보이기 여부 (기본값: true)
- `clickable`: 클릭 가능 여부 (기본값: false)

---

## 위젯 시스템

위젯은 복잡한 UI 컴포넌트입니다. 상태 관리와 이벤트 처리를 포함합니다.

### 버튼 (Button)

클릭 가능한 버튼 위젯입니다.

**JSON 형식:**
```json
{
    "name": "start_button",
    "type": "button",
    "loc": [300, 300, 450, 350],
    "normalImage": "button_normal",
    "pressedImage": "button_pressed",
    "disabledImage": "button_disabled",
    "scale": 1.0,
    "rotate": 0.0,
    "alpha": 1.0,
    "visible": true,
    "parent": "menu_panel"
}
```

**필드 설명:**
- `normalImage`: 평소 상태 이미지 (필수)
- `pressedImage`: 눌렸을 때 이미지 (필수)
- `disabledImage`: 비활성화 이미지 (선택)
- `parent`: 부모 요소 이름 (선택, 위치 상속)

**Lua에서 사용:**
```lua
widget.setButtonCallback("start_button", function()
    print("Button clicked!")
    Scene.showScene("game_scene")
end)

widget.setButtonEnabled("start_button", true)  -- 활성화/비활성화
```

### 텍스트 (Text)

텍스트를 표시하는 위젯입니다.

**JSON 형식:**
```json
{
    "name": "title_text",
    "type": "text",
    "text": "게임 타이틀",
    "loc": [100, 10, 500, 120],
    "textSize": 24,
    "textColor": [255, 255, 255],
    "scale": 1.0,
    "rotate": 0.0,
    "alpha": 1.0,
    "visible": true,
    "clickable": true,
    "parent": "header_panel"
}
```

**필드 설명:**
- `text`: 표시할 텍스트 내용
- `textSize`: 폰트 크기 (기본값: 12)
- `textColor`: 색상 `[R, G, B]` 또는 `[R, G, B, A]` (기본값: [255, 255, 255])
- `loc`: 위치와 크기 (크기는 텍스트에 맞춰 자동 조정됨)

**Lua에서 사용:**
```lua
widget.setText("title_text", "새 텍스트")
local text = widget.getText("title_text")
-- 위젯의 UI element ID를 얻어 ui/애니메이션 API로 조작 가능
local id = widget.getUIElementId("title_text")
if id ~= "" then
    ui.setAlpha(id, 0.8)
    ui.setVisible(id, true)
end
```

### 배경 (Background)

단색 또는 이미지 배경을 표시하는 위젯입니다. 이미지는 stretch, tile, fit, scroll 모드로 표시할 수 있습니다.

**JSON 형식 (색상 배경):**
```json
{
    "name": "background",
    "type": "background",
    "loc": [0, 0, 600, 1000],
    "bgColor": [230, 235, 245, 255],
    "scale": 1,
    "rotate": 0,
    "alpha": 1,
    "visible": true
}
```

**JSON 형식 (이미지 배경):**
```json
{
    "name": "background",
    "type": "background",
    "image": "back.color.wave_sky",
    "imageMode": "stretch",
    "loc": [0, 0, 600, 1000],
    "scale": 1,
    "rotate": 0,
    "alpha": 1,
    "visible": true
}
```

**필드 설명:**
- `bgColor`: 색상 배경 시 `[R, G, B]` 또는 `[R, G, B, A]` (기본값: [0, 0, 0, 255])
- `image`: `resource.json`에 등록된 이미지 이름 (색상 대신 사용)
- `imageMode`: 이미지 표시 모드
  - `"stretch"`: 영역에 맞춰 확대 (기본값)
  - `"tile"`: 바둑판 배열
  - `"fit"`: 비율 유지, 남는 공간은 비움
  - `"scroll_h"`: height에 맞춰 늘리고, 가로로 스크롤
  - `"scroll_v"`: width에 맞춰 늘리고, 세로로 스크롤

**스크롤 모드 (`scroll_h`, `scroll_v`):**
- child 이미지 + AnimationManager를 사용해 duration 기반 애니메이션
- `scrollCycleTime` 또는 `scrollDuration`: 한 방향 이동 소요 시간(초)
- 한쪽 끝 도달 시 반대 방향으로 되돌아오는 ping-pong 방식

```json
{
    "name": "background",
    "type": "background",
    "image": "back.color.wave_sky",
    "imageMode": "scroll_h",
    "scrollCycleTime": 15,
    "loc": [0, 0, 600, 1000],
    "scale": 1,
    "rotate": 0,
    "alpha": 1,
    "visible": true
}
```

### 텍스트 리스트 (TextList)

여러 텍스트 항목을 리스트로 표시하는 위젯입니다. 스크롤 기능을 지원합니다.

**JSON 형식:**
```json
{
    "name": "item_list",
    "type": "textlist",
    "loc": [50, 100, 400, 400],
    "itemHeight": 30,
    "textSize": 16,
    "textColor": [255, 255, 255],
    "scale": 1.0,
    "rotate": 0.0,
    "alpha": 1.0,
    "visible": true
}
```

**필드 설명:**
- `itemHeight`: 각 항목의 높이 (픽셀, 기본값: 20)
- `textSize`: 항목 폰트 크기 (기본값: 12)
- `textColor`: 항목 색상 `[R, G, B]` 또는 `[R, G, B, A]` (기본값: [255, 255, 255])

**스크롤 기능:**
- 마우스 클릭 후 드래그로 스크롤 가능
- 위젯 영역을 넘어서는 텍스트는 자동으로 클리핑됨
- 스크롤 범위는 항목 수와 위젯 높이에 따라 자동 계산

**Lua에서 사용:**
```lua
widget.addListItem("item_list", "항목 1")
widget.addListItem("item_list", "항목 2")
widget.clearListItems("item_list")
```

### 텍스트 입력 (EditText)

사용자로부터 텍스트 입력을 받는 위젯입니다.

**JSON 형식:**
```json
{
    "name": "input_field",
    "type": "edittext",
    "loc": [50, 200, 350, 250],
    "placeholder": "입력하세요...",
    "textSize": 16,
    "textColor": [255, 255, 255],
    "bgColor": [50, 50, 50],
    "borderColor": [100, 100, 100],
    "maxLength": 100,
    "multiLine": false,
    "scale": 1.0,
    "rotate": 0.0,
    "alpha": 1.0,
    "visible": true,
    "clickable": true
}
```

**필드 설명:**
- `placeholder`: 입력 전 표시할 텍스트 (기본값: 빈 문자열)
- `textSize`: 폰트 크기 (기본값: 16)
- `textColor`: 텍스트 색상 `[R, G, B]` 또는 `[R, G, B, A]` (기본값: [255, 255, 255])
- `bgColor`: 배경 색상 `[R, G, B]` 또는 `[R, G, B, A]` (기본값: [0, 0, 0])
- `borderColor`: 테두리 색상 `[R, G, B]` 또는 `[R, G, B, A]` (기본값: [255, 255, 255])
- `maxLength`: 최대 입력 길이 (기본값: 0 = 무제한)
- `multiLine`: 여러 줄 입력 허용 여부 (기본값: false)

**Lua에서 사용:**
```lua
-- 텍스트 가져오기
local text = widget.getEditText("input_field")

-- 텍스트 설정
widget.setEditText("input_field", "새 텍스트")

-- 클릭 시 포커스 자동 설정 (위젯이 자동 처리)
```

**입력 기능:**
- 클릭 시 자동으로 포커스 획득
- 텍스트 입력: 일반 키 입력
- 백스페이스: 커서 앞 문자 삭제
- Delete: 커서 뒤 문자 삭제
- 화살표 키: 커서 이동 (추후 구현 예정)

### 채팅 리스트 (ChatList)

채팅 앱처럼 메시지를 왼쪽/오른쪽 정렬로 표시하는 위젯입니다. 각 메시지는 아이콘과 텍스트로 구성됩니다.

**JSON 형식:**
```json
{
    "name": "chat_list",
    "type": "chatlist",
    "loc": [50, 60, 750, 550],
    "itemHeight": 60,
    "textSize": 14,
    "textColor": [255, 255, 255],
    "iconSize": 40,
    "iconTextSpacing": 10,
    "messages": [
        {
            "text": "안녕하세요!",
            "icon": "like",
            "alignment": "left"
        },
        {
            "text": "네, 안녕하세요!",
            "icon": "robot",
            "alignment": "right"
        }
    ],
    "scale": 1.0,
    "rotate": 0.0,
    "alpha": 1.0,
    "visible": true,
    "clickable": true
}
```

**필드 설명:**
- `itemHeight`: 각 메시지 항목의 높이 (픽셀, 기본값: 60)
- `textSize`: 메시지 폰트 크기 (기본값: 14)
- `textColor`: 메시지 텍스트 색상 `[R, G, B]` 또는 `[R, G, B, A]` (기본값: [255, 255, 255])
- `iconSize`: 아이콘 크기 (픽셀, 기본값: 40)
- `iconTextSpacing`: 아이콘과 텍스트 간 간격 (픽셀, 기본값: 10)
- `messages`: 초기 메시지 목록 (각 메시지는 `text`, `icon`, `alignment` 속성 포함)
  - `alignment`: `"left"` 또는 `"right"` (기본값: `"left"`)

**스크롤 기능:**
- 마우스 클릭 후 드래그로 스크롤 가능
- 위젯 영역을 넘어서는 메시지는 자동으로 클리핑됨
- 새 메시지 추가 시 Lua API에서 자동으로 맨 아래로 스크롤됨

**Lua에서 사용:**
```lua
-- 메시지 추가 (자동으로 맨 아래로 스크롤)
widget.addChatMessage("chat_list", "안녕하세요!", "like", "left")
widget.addChatMessage("chat_list", "네, 안녕하세요!", "robot", "right")
```

**참고:** 
- `addChatMessage`는 메시지 추가 후 자동으로 `scrollToBottom()`을 호출합니다.
- 아이콘은 `resource.json`에 등록된 텍스처 이름을 사용합니다.

### 섹션 그리드 (SectionGrid)

헤더와 이미지를 섹션별로 그리드로 표시하는 위젯입니다.

**JSON 형식:**
```json
{
    "name": "card_grid",
    "type": "sectiongrid",
    "loc": [20, 100, 580, 970],
    "columns": 3,
    "cellWidth": 180,
    "cellHeight": 240,
    "cellMargin": 8,
    "headerMarginV": 8,
    "scale": 1.0,
    "rotate": 0.0,
    "alpha": 1.0,
    "visible": true,
    "clickable": true
}
```

**필드 설명:**
- `columns`: 열 개수 (기본값: 2)
- `cellWidth`: 셀 너비 (픽셀, 기본값: 100)
- `cellHeight`: 셀 높이 (픽셀, 기본값: 100)
- `cellMargin`: 셀 간 마진 (픽셀, 기본값: 8)
- `headerMarginV`: 헤더와 셀 사이 세로 간격 (픽셀, 기본값: 8)

**스크롤 기능:**
- 마우스 드래그로 스크롤 가능
- 위젯 영역을 넘어서는 내용은 자동으로 클리핑됨

**Lazy loading:**
- 스크롤 시 뷰포트에 보이는 항목만 UI·텍스처 로드 (메모리·초기 로딩 최적화)

**오버레이:**
- `addSectionOverlay`로 오버레이 정의 (여러 개 등록 가능)
- `addSectionCard` 시 overlayId 지정하여 해당 오버레이 사용 (빈 문자열/nil = 오버레이 없음)
- bool 전달 시: true → "locked" 오버레이, false → 없음 (하위 호환)

**Lua에서 사용:**
```lua
-- 오버레이 정의 (overlayId, imageName, height 선택, 기본 36)
widget.addSectionOverlay("card_grid", "locked", "ui.btn_luxury_white")

-- 헤더 추가 (이미지 이름만)
widget.addSectionHeader("card_grid", "ui.btn_luxury_white")

-- 항목 추가 (imageName, overlayId 또는 bool)
widget.addSectionCard("card_grid", "img_001", nil)      -- 오버레이 없음
widget.addSectionCard("card_grid", "img_002", "locked")  -- locked 오버레이

-- 전체 초기화 (스크롤 위치도 0으로 리셋)
widget.clearSectionGrid("card_grid")

-- 헤더 배경 이미지 가로 비율 (0~1, 기본 0.5)
widget.setSectionGridHeaderScale("card_grid", 0.5)

-- items 추가 후 반드시 한 번 호출 (성능 최적화)
widget.layoutSectionGrid("card_grid")
```

**사용 패턴:**
```lua
function init()
    widget.clearSectionGrid("card_grid")
    widget.addSectionOverlay("card_grid", "locked", "ui.btn_luxury_white")
    
    local currentSection = nil
    for _, item in ipairs(itemsData) do
        if currentSection ~= item.section then
            currentSection = item.section
            widget.addSectionHeader("card_grid", item.headerImage or "ui.btn_luxury_white")
        end
        local overlayId = item.locked and "locked" or nil
        widget.addSectionCard("card_grid", item.imageName, overlayId)
    end
    
    widget.layoutSectionGrid("card_grid")  -- 마지막에 한 번만
end
```


### 토스트 (Toast)

일시적으로 메시지를 표시하는 위젯입니다. (예: "다음 업데이트때 지원")

**JSON 형식:**
```json
{
    "name": "toast_message",
    "type": "toast",
    "loc": [0, 460, 600, 540],
    "textSize": 18,
    "textColor": [255, 255, 255, 255],
    "visible": false
}
```

**필드 설명:**
- `loc`: 배경 영역 `[x1, y1, x2, y2]`. 화면 가로 전체로 쓰려면 `[0, y1, 600, y2]` 처럼 `screen_width`에 맞춰 지정 (권장)
- `textSize`: 메시지 폰트 크기 (기본값: 18)
- `textColor`: 텍스트 색상 (기본값: [255, 255, 255, 255])

토스트는 항상 다른 UI보다 위에 그려지며(alwaysOnTop), JSON 순서나 코드에서 나중에 추가되는 요소와 무관하게 최상위에 표시됩니다.

**Lua에서 사용:**
```lua
widget.showToast("toast_message", "다음 업데이트때 지원", 3000)  -- 3초 표시
widget.hideToast("toast_message")  -- 즉시 숨김
```

---

## 타일맵 시스템

타일맵은 RPG 게임 등에서 사용하는 그리드 기반 맵 시스템입니다.

### 맵 파일 생성

맵 파일은 `resource/` 폴더에 JSON 형식으로 저장합니다.

**맵 파일 형식 (`resource/map_level1.json`):**
```json
{
    "type": "map",
    "tileset": "grass_tileset",
    "tileSize": 32,
    "tilesetColumns": 8,
    "mapWidth": 50,
    "mapHeight": 50,
    "tileData": [
        [1, 2, 3, 4, ...],
        [5, 6, 7, 8, ...],
        ...
    ]
}
```

**중요:** 맵 파일 내부에 `"type": "map"` 필드가 반드시 필요합니다.

**필드 설명:**
- `tileset`: 타일셋 텍스처 이름 (`resource.json`에 등록되어 있어야 함)
- `tileSize`: 타일 크기 (픽셀)
- `tilesetColumns`: 타일셋의 열 개수 (타일 ID 계산용)
- `mapWidth`: 맵 너비 (타일 개수)
- `mapHeight`: 맵 높이 (타일 개수)
- `tileData`: 타일 ID 배열 `[y][x]` (0은 빈 타일)

**타일 ID 계산:**
- 타일셋이 8열로 구성되어 있다면:
  - 타일 ID 0: 첫 번째 타일 (0, 0)
  - 타일 ID 1: 두 번째 타일 (1, 0)
  - 타일 ID 8: 두 번째 줄 첫 번째 타일 (0, 1)
  - 공식: `타일 위치 = (ID % columns, ID / columns)`

### 리소스에 맵 파일 등록

리소스 매니페스트(예: `setting.json`의 `resource_file`로 지정한 JSON)에 맵 파일을 등록합니다:

```json
{
    "initial_scene": "scene_menu",
    "resource_folder": "resource_example",
    "resources": [
        {"name": "level1_map", "type": "json", "path": "map_level1.json"},
        {"name": "grass_tileset", "type": "image", "path": "tileset_grass.png"}
    ]
}
```

**참고:** 
- 맵 파일은 `type: "json"`으로 등록합니다. 경로는 `resource_folder`가 있으면 그 뒤에 붙습니다.
- JSON 파일 내부에 `"type": "map"` 필드가 있어야 합니다.
- Lazy loading 방식으로 요청 시 파싱됩니다 (초기 로딩 속도 향상).

### 씬에서 타일맵 사용

**방법 1: 맵 파일 사용 (권장)**
```json
{
    "name": "game_map",
    "type": "tilemap",
    "map": "level1_map",
    "offset": [0, 0]
}
```

**방법 2: 직접 지정 (작은 맵용)**
```json
{
    "name": "small_map",
    "type": "tilemap",
    "tileset": "grass_tileset",
    "tileSize": 32,
    "tilesetColumns": 8,
    "mapWidth": 10,
    "mapHeight": 10,
    "tileData": [
        [1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
        [1, 2, 2, 2, 2, 2, 2, 2, 2, 1],
        ...
    ],
    "offset": [0, 0]
}
```

**필드 설명:**
- `map`: ResourceManager에 등록된 맵 이름 (방법 1)
- `tileset`, `tileSize`, `tilesetColumns`, `mapWidth`, `mapHeight`, `tileData`: 직접 지정 (방법 2)
- `offset`: 렌더링 오프셋 (카메라 위치, 기본값: [0, 0])
- `loc`: 위젯의 위치와 크기 (위젯 위치를 고려하여 타일 렌더링됨)

### 코드에서 타일맵 조작

```cpp
// 위젯 가져오기
auto* tilemap = widgetManager.getWidget("game_map");
if (tilemap) {
    auto* tm = dynamic_cast<TilemapWidget*>(tilemap);
    
    // 카메라 이동
    tm->setOffset(cameraX, cameraY);
    
    // 타일 변경
    tm->setTile(10, 10, 5);  // (10, 10) 위치에 타일 ID 5 설정
    
    // 타일 조회
    int tileId = tm->getTile(10, 10);
    
    // 맵 크기 조회
    int width = tm->getMapWidth();
    int height = tm->getMapHeight();
    int tileSize = tm->getTileSize();
}
```

---

## 부모-자식 관계

UI 요소는 부모-자식 관계를 가질 수 있습니다. 자식은 부모의 위치, 스케일, 회전을 상속받습니다.

### 특징

- **다단계 지원**: 부모의 부모, 그 부모의 부모 등 무제한 깊이 지원
- **위치 상속**: 자식의 로컬 위치는 부모 좌상단을 (0, 0) 기준으로 한 상대 좌표
- **스케일 상속**: 자식의 최종 스케일 = 자식 스케일 × 부모 스케일 × 조상 스케일...
- **회전 상속**: 자식의 최종 회전 = 자식 회전 + 부모 회전 + 조상 회전...
- **alpha 상속**: 자식의 최종 alpha = 자식 alpha × 부모 alpha × 조상 alpha... (렌더 시 적용)
- **클리핑**: 기본적으로 자식은 부모 영역으로 클리핑되지만, 부모나 본인이 스케일/회전 중일 때는 클리핑이 비활성화되어 잘리지 않음

### 사용 예시

```json
{
    "name": "menu_panel",
    "type": "image",
    "image": "panel_bg",
    "loc": [100, 100, 500, 400],
    "scale": 1.0,
    "rotate": 0.0
},
{
    "name": "close_button",
    "type": "button",
    "normalImage": "close_normal",
    "pressedImage": "close_pressed",
    "loc": [10, 10, 50, 50],
    "parent": "menu_panel",
    "scale": 1.0,
    "rotate": 0.0
}
```

위 예시에서 `close_button`은 `menu_panel`의 자식이므로:
- `menu_panel`이 (100, 100)에 있으면
- `close_button`은 실제로 (110, 110)에 렌더링됩니다 (부모 위치 + 로컬 위치)
- `menu_panel`의 스케일이 1.5이면 `close_button`도 1.5배 스케일됨
- `menu_panel`이 45도 회전하면 `close_button`도 함께 45도 회전됨

### 다단계 예시

```json
{
    "name": "container",
    "type": "image",
    "loc": [50, 50, 400, 300]
},
{
    "name": "panel",
    "type": "image",
    "loc": [10, 10, 200, 150],
    "parent": "container"
},
{
    "name": "button",
    "type": "button",
    "loc": [5, 5, 50, 30],
    "parent": "panel"
}
```

- `button`의 부모는 `panel`
- `panel`의 부모는 `container`
- `button`은 `panel`과 `container`의 위치/스케일/회전을 모두 상속받음

### 코드에서 부모 설정

```cpp
// 부모 설정
uiManager.setParent("child_element", "parent_element");

// 부모 제거
uiManager.removeParent("child_element");
```

**주의**: 순환 참조는 자동으로 방지됩니다 (예: A → B → C → A).

---

## 애니메이션

애니메이션은 UI 요소의 속성을 시간에 따라 변경합니다.

### Lua에서 사용

```lua
-- 애니메이션 생성 (UI 요소 이름으로 Animator 생성)
local anim = Animator("element_name")
anim:rotate(0, 360, 1000)      -- 0도에서 360도로, 1초
    :move(100, 50, 500)         -- (100, 50) 이동, 0.5초
    :resize(200, 200, 300)      -- 크기 변경, 0.3초
    :repeat_anim(-1)            -- 무한 반복
    :callback(function()        -- 완료 콜백
        print("Animation complete!")
    end)

-- 애니메이션 등록
-- 첫 번째 파라미터는 UI 요소 이름이어야 함 (애니메이션 이름이 아님!)
-- Animator 생성 시 사용한 UI 요소 이름과 동일해야 함
animation.add("element_name", anim:get())
```

**위젯에 애니메이션 적용:** 위젯은 UiManager에 자동 생성 ID로 등록되므로, `widget.getUIElementId("widget_name")`으로 ID를 얻은 뒤 `Animator(id)`로 애니메이션을 만들고 `animation.add(id, anim:get())`로 등록합니다.

**지원 애니메이션:**
- `rotate(startDeg, endDeg, durationMs)`: 회전
- `move(dx, dy, durationMs)`: 상대 이동
- `moveto(x, y, durationMs)`: 절대 이동
- `resize(w, h, durationMs)`: 크기 변경
- `alpha(startAlpha, endAlpha, durationMs)`: 투명도 (0~1)
- `changeTexture(imageName)`: 텍스처 변경
- `delay(durationMs)`: 지연
- `repeat_anim(count)`: 반복 (-1 = 무한)
- `callback(function)`: 완료 콜백

---

## Lua 스크립팅

씬별 Lua 스크립트를 작성하여 게임 로직을 구현할 수 있습니다.

### 씬 파일에서 스크립트 지정

```json
{
    "code": "scene_test_code",
    "ui": [...]
}
```

리소스 매니페스트(`setting.json`의 `resource_file`에 지정한 JSON)에 스크립트 파일을 등록:
```json
{
    "resources": [
        {"name": "scene_test_code", "type": "text", "path": "scene_test_code.lua"}
    ]
}
```
(`resource_folder`가 있으면 `path` 앞에 붙습니다.)

### 이벤트 핸들러

```lua
-- 씬 로드 시 한 번 호출
function init()
    print("Scene initialized!")
    
    -- 버튼 콜백 설정
    widget.setButtonCallback("start_button", function()
        Scene.showScene("game_scene")
    end)
end

-- 매 프레임 호출
function update()
    -- 게임 로직 업데이트
end

-- 입력 이벤트 처리
function keyPressed(elementName, type, keycode, x, y)
    if type == "mouse_click" then
        if elementName == "start_button" then
            print("Start button clicked!")
        end
    end
end
```

### UI 조작 API

```lua
-- 이미지 변경
ui.changeImage("element_name", "new_texture")

-- 위치 이동
ui.move("element_name", dx, dy)           -- 상대 이동
ui.moveto("element_name", x, y)           -- 절대 이동
ui.resize("element_name", w, h)          -- 크기 변경
ui.setRect("element_name", x, y, w, h)  -- 전체 rect 설정

-- 변환 속성
ui.setAlpha("element_name", 0.5)
ui.setVisible("element_name", true)   -- 표시/숨김
ui.setAlwaysOnTop("element_name", true)  -- true면 항상 최상위에 렌더 (토스트·모달 등)
ui.setRotate("element_name", 45)
ui.setScale("element_name", 1.5)

-- Getter
local x = ui.getLeft("element_name")
local y = ui.getTop("element_name")
local w = ui.getWidth("element_name")
local h = ui.getHeight("element_name")
local alpha = ui.getAlpha("element_name")
```

### 위젯 조작 API

```lua
-- 위젯의 UI element ID 조회 (애니메이션/ui.setAlpha/ui.setVisible 등에 사용)
local id = widget.getUIElementId("widget_name")
if id ~= "" then
    ui.setAlpha(id, 0.5)
    animation.add(id, anim:get())
end

-- 버튼
widget.setButtonCallback("button_name", function()
    print("Clicked!")
end)
widget.setButtonEnabled("button_name", true)

-- 토스트 메시지
widget.showToast("toast_message", "표시할 텍스트", 3000)  -- durationMs 선택 (기본 3000)
widget.hideToast("toast_message")

-- 텍스트 리스트
widget.addListItem("list_name", "항목 텍스트")
widget.clearListItems("list_name")

-- 채팅 리스트
widget.addChatMessage("chat_list", "메시지 텍스트", "icon_name", "left")
-- alignment: "left" 또는 "right" (기본값: "left")
-- 메시지 추가 후 자동으로 맨 아래로 스크롤됨

-- 섹션 그리드
widget.addSectionHeader("card_grid", "ui.btn_luxury_white")
widget.addSectionCard("card_grid", "img_id", "locked")  -- overlayId 또는 bool
widget.clearSectionGrid("card_grid")
widget.setSectionGridHeaderScale("card_grid", 0.5)
widget.layoutSectionGrid("card_grid")  -- add 후 마지막에 한 번만 호출

-- 텍스트 입력
local text = widget.getEditText("input_name")
widget.setEditText("input_name", "새 텍스트")

-- 씬 전환
Scene.showScene("scene_name")
```

---

## 입력 포커스 관리

입력 포커스는 현재 키보드 입력을 받을 위젯을 결정합니다. 주로 `EditTextWidget`에서 사용됩니다.

### 동작 방식

1. **자동 포커스 관리**: `WidgetManager`가 중앙에서 포커스를 관리합니다.
2. **클릭 시 포커스**: 위젯을 클릭하면 자동으로 포커스를 획득합니다.
3. **SDL_TextInput 자동 제어**: 포커스가 있으면 `SDL_StartTextInput()`, 없으면 `SDL_StopTextInput()` 자동 호출
4. **이벤트 우선순위**: 포커스된 위젯이 `SDL_TEXTINPUT` 이벤트를 우선 처리합니다.

### 포커스를 받을 수 있는 위젯

현재는 `EditTextWidget`만 포커스를 받을 수 있습니다 (`canReceiveFocus()` 반환 `true`).

### Lua에서 포커스 제어

현재는 Lua API에서 직접 포커스를 제어할 수 없습니다. 위젯 클릭 시 자동으로 처리됩니다.

### 이벤트 처리 흐름

```
SDL 이벤트 발생
    ↓
WidgetManager::handleEvents()
    ↓
포커스된 위젯이 있으면?
    ├─ YES → 포커스된 위젯에 이벤트 전달
    │         (SDL_TEXTINPUT은 포커스된 위젯만 처리)
    └─ NO  → 모든 위젯에 이벤트 전달
    ↓
일반 UI 요소 처리
    ↓
Lua keyPressed() 호출
    (위젯에서 소비되지 않은 이벤트만)
```

---

## 예시: 간단한 메뉴 씬

(해상도는 `setting.json`의 `width`/`height`로 결정됩니다.)

```json
{
    "code": "menu_code",
    "ui": [
        {
            "name": "background",
            "type": "image",
            "image": "menu_bg",
            "loc": [0, 0, 800, 600]
        },
        {
            "name": "menu_panel",
            "type": "image",
            "image": "panel",
            "loc": [200, 150, 600, 450]
        },
        {
            "name": "title",
            "type": "text",
            "text": "게임 타이틀",
            "loc": [250, 180, 550, 220],
            "textSize": 32,
            "textColor": [255, 255, 0],
            "parent": "menu_panel"
        },
        {
            "name": "start_button",
            "type": "button",
            "normalImage": "button_normal",
            "pressedImage": "button_pressed",
            "loc": [50, 100, 350, 150],
            "parent": "menu_panel"
        },
        {
            "name": "exit_button",
            "type": "button",
            "normalImage": "button_normal",
            "pressedImage": "button_pressed",
            "loc": [50, 200, 350, 250],
            "parent": "menu_panel"
        }
    ]
}
```

**Lua 스크립트 (`menu_code.lua`):**
```lua
function init()
    widget.setButtonCallback("start_button", function()
        Scene.showScene("game_scene")
    end)
    
    widget.setButtonCallback("exit_button", function()
        -- 종료 로직
    end)
end
```

---

## 팁과 모범 사례

### 1. 리소스 관리
- `setting.json`의 `resource_file`에 지정한 리소스 매니페스트에 이미지/씬/스크립트를 등록
- 큰 맵은 별도 맵 파일로 분리하고 `type: "json"`으로 등록
- 텍스처 이름은 명확하게 지정

### 2. 부모-자식 관계 활용
- 관련된 UI 요소를 그룹화
- 패널을 부모로 사용하여 일괄 이동 가능

### 3. 타일맵 최적화
- 큰 맵은 맵 파일로 분리
- 화면에 보이지 않는 타일은 렌더링되지 않음 (자동 처리)
- 위젯 위치와 크기를 고려하여 렌더링 (전체 화면이 아닌 위젯 영역만)

### 4. 성능 고려
- 타일맵은 `TilemapWidget` 사용 (직접 렌더링)
- 작은 UI 요소는 일반 `image` 타입 사용
- 텍스트 위젯은 항상 개별 텍스처로 생성 (캐싱 없음)
- 위젯 렌더링 시 클리핑이 자동으로 적용되어 불필요한 렌더링 방지

### 5. 텍스처 ID 관리
- 동적 생성 텍스처는 자동 ID 사용 (`registerTexture(texture)`)
- 이름 지정 등록은 JSON에서 로드한 정적 텍스처 참조용으로만 사용
- 텍스트 위젯은 항상 개별 텍스처 (ID 공유 없음)

---

**마지막 업데이트**: 2025-02-14
**엔진 버전**: 개발 중

---

## 최근 업데이트 (2025-02-14)

### UI / 이미지
- **3패치 (가로 3등분)**: 이미지를 가로로 좌/중/우 3등분해 중간만 늘리는 방식 지원
  - **image**: JSON에 `"useThreePatch": true` 지정
  - **button**: `"useThreePatch": true` (useNinePatch와 동시 지정 시 9패치 우선)
  - **backgroundtext**: `"backgroundImageThreePatch": true`
  - **bannerlist / upgradelist** 등: 항목 배경/버튼에 해당 옵션 지원
- **9패치·3패치 유지**: `ui.changeImage()`로 이미지를 바꿀 때도, 해당 요소가 원래 9패치/3패치였으면 새 텍스처를 같은 방식으로 다시 생성해 적용 (카드팩 배경 등에서 동작)
- **alwaysOnTop (최상위 렌더)**: 토스트·모달 등 항상 맨 위에 그려야 하는 요소용
  - `UIElement`에 `alwaysOnTop` 플래그 추가
  - `ui.setAlwaysOnTop(name, true)` 로 설정 (Lua API는 동일 이름으로 추가 가능)
  - 렌더는 1패스(일반) → 2패스(alwaysOnTop) 순으로 수행

### 위젯
- **BannerList itemMargin**: 리스트 항목 사이 세로 간격
  - JSON에 `"itemMargin": 12` (픽셀) 지정
- **Toast**
  - **가로 전체**: 씬 JSON에서 토스트 `loc`를 `[0, y1, 600, y2]` 처럼 화면 폭(600)에 맞춰 선언해 가로로 꽉 차게 사용 (권장)
  - **최상위 표시**: 토스트 배경·텍스트에 `alwaysOnTop`이 자동 설정되어, 코드에서 나중에 추가되는 UI보다 항상 위에 그려짐

---

## 최근 업데이트 (2025-02)

### 주요 추가 기능
- **SectionGridWidget**: 헤더 + 섹션 그리드 위젯
  - `addSectionOverlay`, `addSectionHeader(imageName)`, `addSectionCard`, `clearSectionGrid`, `setSectionGridHeaderScale`, `layoutSectionGrid`
  - 헤더: 이미지만 (텍스트 제거), 오버레이 공용 정의
  - Lazy loading: 뷰포트에 보이는 항목만 UI·텍스처 로드
  - `clear()` 시 스크롤 위치 0으로 리셋
  - 마우스 드래그 스크롤 지원
- **widget.getUIElementId(name)**: 위젯 이름으로 UiManager element ID 조회. `ui.setAlpha`, `ui.setVisible`, `animation.add` 등에 사용
- **ui.setVisible(name, visible)**: UI 요소 표시/숨김 설정
- **설정 (setting.json)**: 내부 해상도·창 크기·리소스 파일 분리, 논리 해상도 스케일링 및 레터박스
- **EditTextWidget**: 텍스트 입력 위젯 추가
  - 포커스 기반 입력 처리
  - 커서 표시 및 편집 기능
  - Placeholder 텍스트 지원
- **TextListWidget**: 텍스트 리스트 위젯 추가
  - 마우스 드래그 스크롤 지원
  - 위젯 영역 클리핑 자동 적용
- **ChatListWidget**: 채팅 리스트 위젯 추가
  - 왼쪽/오른쪽 정렬 메시지 표시
  - 아이콘과 텍스트를 개별 UIElement로 관리
  - 스크롤 및 클리핑 지원
  - Lua API에서 메시지 추가 시 자동 스크롤 기능
- **포커스 관리 시스템**: WidgetManager에서 중앙 집중식 포커스 관리
- **씬 전환 개선**: 지연 로딩 방식으로 크래시 방지
- **타일맵 위젯 개선**: 위젯 위치와 크기를 고려한 렌더링
- **렌더링 시스템 개선**: 
  - UiManager에서 모든 UIElement 렌더링 및 부모-자식 클리핑 처리
  - WidgetManager에서 위젯별 클리핑 자동 적용

