#include "EditTextWidget.h"
#include "../../ui/uiManager.h"
#include "../../rendering/TextRenderer.h"
#include "../../rendering/ImageRenderer.h"
#include "../../utils/logger.h"
#include <algorithm>

EditTextWidget::EditTextWidget(UiManager* uiMgr, ResourceManager* resMgr,
                               SDL_Renderer* sdlRenderer, TextRenderer* txtRenderer,
                               const std::string& placeholderText, int size,
                               SDL_Color textColor, SDL_Color bgColor, SDL_Color borderColor,
                               int maxLen, bool multiLine)
    : UIWidget(uiMgr, resMgr),
      text(""),  // 초기 텍스트는 비어있음
      placeholder(placeholderText),
      cursorPosition(0),
      hasFocus(false),
      showCursor(true),
      cursorBlinkTimer(0.0f),
      renderer(sdlRenderer),
      textRenderer(txtRenderer),
      textTexture(nullptr),
      currentTextureName(""),
      maxLength(maxLen),
      multiline(multiLine),
      textColor(textColor),
      backgroundColor(bgColor),
      borderColor(borderColor),
      fontSize(size),
      padding(4),
      uiElementId("") {
    
    // UIElement 생성 (배경 + 텍스트)
    if (uiElementId.empty()) {
        // 기본 크기로 UIElement 생성 (나중에 JSON에서 크기 설정됨)
        UIElement elem;
        elem.name = "";  // 빈 문자열로 전달하면 자동 ID 생성
        elem.texture = nullptr;
        elem.rect = SDL_Rect{0, 0, 200, fontSize + padding * 2};
        elem.visible = true;
        elem.clickable = true;
        uiElementId = uiManager->addUIAndGetId(elem);
    }
    
    textRect = {padding, padding, 0, 0};
    updateTexture();
}

EditTextWidget::~EditTextWidget() {
    // ResourceManager에 텍스처 해제 요청
    if (!currentTextureName.empty() && resourceManager) {
        resourceManager->unregisterTexture(currentTextureName);
        currentTextureName = "";
        textTexture = nullptr;
    }
    
    // SDL 텍스트 입력 모드 종료
    if (hasFocus) {
        SDL_StopTextInput();
    }
}

void EditTextWidget::setFocus(bool focus) {
    if (hasFocus == focus) return;
    
    hasFocus = focus;
    
    if (hasFocus) {
        SDL_StartTextInput();  // 텍스트 입력 모드 시작
        cursorBlinkTimer = 0.0f;
        showCursor = true;
        cursorPosition = static_cast<int>(text.length());  // 커서를 텍스트 끝으로
    } else {
        SDL_StopTextInput();   // 텍스트 입력 모드 종료
        showCursor = false;
    }
    
    updateTexture();  // 포커스 상태에 따라 커서 표시 업데이트
}

// isPointInside()는 베이스 클래스의 공통 구현 사용

void EditTextWidget::handleEvent(const SDL_Event& event) {
    if (!hasFocus) return;  // 포커스가 없으면 이벤트 무시
    
    // SDL_TEXTINPUT: 실제 텍스트 입력 (한글, 이모지 등)
    if (event.type == SDL_TEXTINPUT) {
        insertText(event.text.text);
        updateTexture();
        return;  // 이벤트 소비
    }
    
    // SDL_KEYDOWN: 특수 키 처리
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
            case SDLK_BACKSPACE:
                deleteChar();
                updateTexture();
                break;
            case SDLK_DELETE:
                deleteCharForward();
                updateTexture();
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if (multiline) {
                    insertText("\n");
                    updateTexture();
                }
                break;
            case SDLK_LEFT:
                moveCursor(-1);
                updateTexture();
                break;
            case SDLK_RIGHT:
                moveCursor(1);
                updateTexture();
                break;
            case SDLK_HOME:
                setCursorPosition(0);
                updateTexture();
                break;
            case SDLK_END:
                setCursorPosition(static_cast<int>(text.length()));
                updateTexture();
                break;
            case SDLK_TAB:
                // Tab은 포커스 이동으로 사용할 수 있지만, 여기서는 무시
                break;
            default:
                // 다른 키는 무시 (단축키로 전달되지 않도록)
                break;
        }
        return;  // 이벤트 소비
    }
}

void EditTextWidget::update(float deltaTime) {
    updateCursorBlink(deltaTime);
}

void EditTextWidget::updateCursorBlink(float deltaTime) {
    if (!hasFocus) {
        showCursor = false;
        return;
    }
    
    cursorBlinkTimer += deltaTime;
    const float blinkInterval = 500.0f;  // 500ms마다 깜빡임
    
    if (cursorBlinkTimer >= blinkInterval) {
        cursorBlinkTimer = 0.0f;
        showCursor = !showCursor;
        updateTexture();  // 커서 표시 업데이트
    }
}

void EditTextWidget::setText(const std::string& newText) {
    text = newText;
    cursorPosition = std::min(cursorPosition, static_cast<int>(text.length()));
    updateTexture();
}

void EditTextWidget::insertText(const std::string& str) {
    if (maxLength > 0 && static_cast<int>(text.length() + str.length()) > maxLength) {
        return;  // 최대 길이 초과
    }
    
    text.insert(cursorPosition, str);
    cursorPosition += static_cast<int>(str.length());
    updateTexture();
}

void EditTextWidget::deleteChar() {
    if (cursorPosition > 0) {
        text.erase(cursorPosition - 1, 1);
        cursorPosition--;
        updateTexture();
    }
}

void EditTextWidget::deleteCharForward() {
    if (cursorPosition < static_cast<int>(text.length())) {
        text.erase(cursorPosition, 1);
        updateTexture();
    }
}

void EditTextWidget::moveCursor(int direction) {
    if (direction < 0) {
        cursorPosition = std::max(0, cursorPosition - 1);
    } else if (direction > 0) {
        cursorPosition = std::min(static_cast<int>(text.length()), cursorPosition + 1);
    }
}

void EditTextWidget::setCursorPosition(int pos) {
    cursorPosition = std::max(0, std::min(pos, static_cast<int>(text.length())));
}

void EditTextWidget::setTextColor(SDL_Color color) {
    textColor = color;
    updateTexture();
}

void EditTextWidget::setBackgroundColor(SDL_Color color) {
    backgroundColor = color;
    updateTexture();
}

void EditTextWidget::setBorderColor(SDL_Color color) {
    borderColor = color;
    updateTexture();
}

void EditTextWidget::setFontSize(int size) {
    fontSize = size;
    updateTexture();
}

void EditTextWidget::updateTexture() {
    if (uiElementId.empty()) return;
    auto* element = uiManager->findElementByName(uiElementId);
    if (!element) {
        Log::error("[EditTextWidget] UIElement not found: ", uiElementId);
        return;
    }
    
    int widgetW = element->rect.w;
    int widgetH = element->rect.h;
    
    // 크기가 0이면 텍스처를 생성하지 않음 (WidgetManager에서 크기를 설정하기 전)
    if (widgetW <= 0 || widgetH <= 0) {
        return;
    }
    
    // 텍스트 영역 계산
    textRect.w = widgetW - padding * 2;
    textRect.h = widgetH - padding * 2;
    
    // ImageRenderer로 배경 + 텍스트 + 커서 그리기
    ImageRenderer imageRenderer(renderer, widgetW, widgetH);
    
    // 배경 그리기
    imageRenderer.drawRect(0, 0, widgetW, widgetH, backgroundColor, true);
    
    // 테두리 그리기
    if (hasFocus) {
        // 포커스가 있으면 테두리를 더 밝게
        SDL_Color focusBorderColor = {
            static_cast<Uint8>(std::min(255, static_cast<int>(borderColor.r) + 50)),
            static_cast<Uint8>(std::min(255, static_cast<int>(borderColor.g) + 50)),
            static_cast<Uint8>(std::min(255, static_cast<int>(borderColor.b) + 50)),
            borderColor.a
        };
        imageRenderer.drawRect(0, 0, widgetW, widgetH, focusBorderColor, false);
    } else {
        imageRenderer.drawRect(0, 0, widgetW, widgetH, borderColor, false);
    }
    
    // 텍스트 그리기
    std::string displayText = text;
    SDL_Color displayColor = textColor;
    
    // 텍스트가 비어있고 포커스가 없으면 placeholder 표시
    if (text.empty() && !hasFocus && !placeholder.empty()) {
        displayText = placeholder;
        // placeholder는 회색으로 표시
        displayColor = {128, 128, 128, textColor.a};
    }
    
    if (!displayText.empty()) {
        // ImageRenderer의 drawText 메서드를 직접 사용 (더 효율적)
        imageRenderer.drawText(displayText, textRect.x, textRect.y, fontSize, displayColor, textRenderer, textRect.w);
    }
    
    // 커서 그리기 (포커스가 있고 깜빡임 상태일 때)
    if (hasFocus && showCursor) {
        // 커서 위치 계산 (간단하게 텍스트 끝에 표시)
        // TODO: 실제 커서 위치 계산 (폰트 메트릭 사용)
        int cursorX = textRect.x;
        if (!text.empty()) {
            // 간단한 근사치 (실제로는 폰트 메트릭 필요)
            cursorX = textRect.x + static_cast<int>(text.length() * fontSize * 0.6f);
        }
        
        // 커서 세로선 그리기
        SDL_Color cursorColor = textColor;
        imageRenderer.drawRect(cursorX, textRect.y, 2, textRect.h, cursorColor, true);
    }
    
    // 최종 텍스처 가져오기
    SDL_Texture* compositeTexture = imageRenderer.getTexture();
    if (compositeTexture) {
        // 기존 텍스처 해제
        if (!currentTextureName.empty()) {
            resourceManager->unregisterTexture(currentTextureName);
            currentTextureName = "";
        }
        
        // 텍스처를 복사하여 등록 (ImageRenderer가 파괴되면 원본 텍스처도 파괴되므로)
        // 텍스처 속성 가져오기
        int texW, texH;
        Uint32 format;
        int access;
        SDL_QueryTexture(compositeTexture, &format, &access, &texW, &texH);
        
        // 새 텍스처 생성 (복사본)
        SDL_Texture* copiedTexture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_TARGET, texW, texH);
        if (copiedTexture) {
            // 원본을 복사본으로 복사
            SDL_Texture* oldTarget = SDL_GetRenderTarget(renderer);
            SDL_SetRenderTarget(renderer, copiedTexture);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, compositeTexture, nullptr, nullptr);
            SDL_SetRenderTarget(renderer, oldTarget);
            
            // 복사본 등록 (자동 ID 생성)
            currentTextureName = resourceManager->registerTexture(copiedTexture);
            textTexture = copiedTexture;
            
            // UIElement 업데이트 (uiElementId 사용)
            uiManager->changeTextureById(uiElementId, currentTextureName);
        } else {
            Log::error("[EditTextWidget] Failed to copy texture: ", SDL_GetError());
        }
    }
}

void EditTextWidget::render(SDL_Renderer* renderer) {
    // 대부분은 UIElement로 렌더링되므로 여기서는 특별한 렌더링만 처리
    // 필요시 커서 깜빡임 등을 여기서 처리할 수 있음
}

