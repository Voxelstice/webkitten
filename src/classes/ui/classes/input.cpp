#include "input.h"
#include <string>

#include "../../../internal/gsgl/gsgl.h"

// TODO: make this compatible with multi-line

Input::Input(GSGL_Font m_font, int m_textSize, std::string m_defaultText, Color defCol, Color curCol) {
    font = m_font;
    defaultText = m_defaultText;
    textSize = m_textSize;

    pos = {0, 0};
    size = {0, 0};

    defaultTextColor = defCol;
    currentTextColor = curCol;

    selectionStart = 0;
    selectionSize = 0;
    offset = {0, 0};
    focus = false;
}

void Input::reset() {
    currentText = "";
    selectionStart = 0;
    selectionSize = 0;
    offset = {0, 0};
    focus = false;
}
void Input::update() {
    bool hovered = gsgl_IsPointInRect(gsgl_GetMousePosition(), pos, size);
    if (hovered == true) {
        if (gsgl_IsMouseButtonPressed(GSGL_LMB) && focus == false) { 
            focus = true;
            selectionStart = int(currentText.length());
            if (resetOnFocus == true) currentText = "";
        }

        gsgl_SetCursor(GSGL_TEXT);
    } else {
        if (gsgl_IsMouseButtonPressed(GSGL_LMB) && focus == true) focus = false;
    }

    if (focus == true) {
        int textLength = (int)currentText.length();

        char chr = gsgl_GetLastChar();
        if (chr != 0 && chr != KEY_BACKSPACE && chr >= 33 && chr <= 126 || chr == KEY_SPACE) {
            if (selectionSize != 0) selectionErase();
            currentText.insert(currentText.begin() + selectionStart, chr);

            selectionStart++;
            textLength++;
        } else if (chr == KEY_BACKSPACE && currentText.length() > 0) {
            selectionStart--;
            selectionErase();
        }

        if (gsgl_IsKeyRepeat(KEY_LEFT) && selectionStart >= 0) {
            if (gsgl_IsKeyDown(KEY_LEFT_SHIFT)) {
                selectionSize--;
            } else if (!gsgl_IsKeyDown(KEY_LEFT_SHIFT) && selectionStart >= 0) {
                selectionStart -= __max(-selectionSize, 1);
                selectionSize = 0;
            }
        } else if (gsgl_IsKeyRepeat(KEY_RIGHT) && selectionStart >= 0) {
            if (gsgl_IsKeyDown(KEY_LEFT_SHIFT)) {
                selectionSize++;
            } else if (!gsgl_IsKeyDown(KEY_LEFT_SHIFT)) {
                selectionStart += __max(selectionSize, 1);
                selectionSize = 0;
            }
        }

        if (gsgl_IsKeyDown(KEY_LEFT_CONTROL)) {
            if (gsgl_IsKeyPressed(KEY_A)) {
                selectionStart = 0;
                selectionSize = textLength;
            } else if (gsgl_IsKeyPressed(KEY_C)) {
                if (selectionSize > 0) gsgl_SetClipboardText(currentText.substr(selectionStart, selectionSize).c_str());
                else if (selectionSize < 0) gsgl_SetClipboardText(currentText.substr(selectionStart+selectionSize, selectionSize).c_str());
            } else if (gsgl_IsKeyPressed(KEY_V)) {
                const char* dataChar = gsgl_GetClipboardText();
                std::string data = dataChar;
                selectionErase();
                currentText.insert(currentText.begin() + selectionStart, data.begin(), data.end());
                selectionStart += (int)data.length();
                textLength += (int)data.length();
                selectionSize = 0;
            }
        }

        if (selectionStart < 0) selectionStart = 0;
        if (selectionStart >= textLength) selectionStart = textLength;

        if (selectionSize >= textLength-selectionStart) selectionSize = textLength-selectionStart;
        if (selectionSize < -selectionStart) selectionSize = -selectionStart;
    } else {
        selectionStart = 0;
        selectionSize = 0;
    }
}
void Input::draw() {
    const char* txtToUse = defaultText.c_str();
    Color colToUse = defaultTextColor;
    if (currentText.length() > 0 || focus == true) {
        txtToUse = currentText.c_str();
        colToUse = currentTextColor;
    }

    gsgl_ScissorsStart(pos.x, pos.y, size.x, size.y);

    gsgl_DrawText(font, txtToUse, pos.x+offset.x, pos.y+offset.y, float(textSize), colToUse);
    if (selectionStart >= 0 && focus == true) {
        if (selectionSize == 0) {
            Vector2i textWidth = gsgl_GetTextSize(font, currentText.substr(0, selectionStart).c_str(), float(textSize));
            gsgl_Rect(pos.x+textWidth.x+1+offset.x, pos.y+offset.y, 1, textSize, currentTextColor);
        } else if (selectionSize > 0) {
            Vector2i textWidthSelectStart = gsgl_GetTextSize(font, currentText.substr(0, selectionStart).c_str(), float(textSize));
            Vector2i textWidthSelectSize = gsgl_GetTextSize(font, currentText.substr(selectionStart, selectionSize).c_str(), float(textSize));

            gsgl_Rect(pos.x+textWidthSelectStart.x+textWidthSelectSize.x+1+offset.x, pos.y+offset.y, 1, textSize, currentTextColor);
            gsgl_Rect(pos.x+textWidthSelectStart.x+1+offset.x, pos.y+int(textSize*0.9)+offset.y, textWidthSelectSize.x, int(textSize*0.1), currentTextColor);
        } else if (selectionSize < 0) {
            Vector2i textWidthSelect = gsgl_GetTextSize(font, currentText.substr(0, selectionStart+selectionSize).c_str(), float(textSize));
            Vector2i textWidthSelectStart = gsgl_GetTextSize(font, currentText.substr(0, selectionStart).c_str(), float(textSize));
            Vector2i textWidthSelectSize = gsgl_GetTextSize(font, currentText.substr(selectionStart+selectionSize, -selectionSize).c_str(), float(textSize));

            gsgl_Rect(pos.x+textWidthSelect.x+offset.x, pos.y+offset.y, 1, textSize, currentTextColor);
            gsgl_Rect(pos.x+textWidthSelectStart.x-textWidthSelectSize.x+offset.x, pos.y+int(textSize*0.9)+offset.y, textWidthSelectSize.x, int(textSize*0.1), currentTextColor);
        }
    }

    gsgl_ScissorsStop();
}

// selection functions
void Input::selectionErase() {
    selectionStart++;

    if (selectionSize == 0) {
        if (selectionStart > 0) selectionStart--;
        currentText.erase(currentText.begin() + selectionStart);
    } else if (selectionSize > 0) {
        if (selectionStart > 0 && selectionSize >= currentText.length()) selectionStart--;
        currentText.erase(currentText.begin() + selectionStart, currentText.begin() + selectionStart + selectionSize);
        selectionSize = 0;
    } else if (selectionSize < 0) {
        if (selectionStart > currentText.length()) selectionStart--;
        currentText.erase(currentText.begin() + selectionStart + selectionSize, currentText.begin() + selectionStart);
        selectionStart += selectionSize;
        selectionSize = 0;
    }
}

// text functions
void Input::setPosition(Vector2i m_pos) {
    pos = m_pos;
}
void Input::setSize(Vector2i m_size) {
    size = m_size;
}
void Input::setRect(Vector2i m_pos, Vector2i m_size) {
    setPosition(m_pos);
    setSize(m_size);
}

void Input::setDefaultText(std::string txt) {
    defaultText = txt;
}
std::string Input::getText() {
    return currentText;
}

void Input::doFocus(bool state) {
    focus = state;
}