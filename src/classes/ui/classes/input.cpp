#include "input.h"
#include <string>

#include "../../../internal/gsgl/gsgl.h"

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
    focus = false;
}

void Input::reset() {
    currentText = "";
    selectionStart = 0;
    selectionSize = 0;
    focus = false;
}
void Input::update() {
    bool hovered = gsgl_IsPointInRect(gsgl_GetMousePosition(), pos, size);
    if (hovered == true) {
        if (gsgl_IsMouseButtonPressed(GSGL_LMB) && focus == false) { 
            focus = true;
            if (resetOnFocus == true) currentText = "";
        }
    } else {
        if (gsgl_IsMouseButtonPressed(GSGL_LMB) && focus == true) focus = false;
    }

    if (focus == true) {
        char chr = gsgl_GetLastChar();
        if (chr != 0 && chr != 8 && chr >= 39 && chr <= 122 || chr == 32) { // 8 is backspace, 32 is space
            std::string lastChar = "";
            lastChar = chr;
            currentText.append(lastChar);
        } else if (chr == 8 && currentText.length() > 0) {
            if (selectionStart == 0) {
                currentText.erase(currentText.end() - 1);
            } else {
                if (selectionSize == 0) {
                    currentText.erase(currentText.begin() + (selectionStart - 1));
                } else if (selectionSize > 0) {
                    currentText.erase(currentText.begin() + (selectionStart - 1), currentText.begin() + selectionSize);
                }
            }
        }
    } else {
        selectionStart = 0;
        selectionSize = 0;
    }
}
void Input::draw() {
    const char* txtToUse = defaultText.c_str();
    Color colToUse = defaultTextColor;
    if (currentText.length() > 0) {
        txtToUse = currentText.c_str();
        colToUse = currentTextColor;
    }

    gsgl_ScissorsStart(pos.x, pos.y, size.x, size.y);

    gsgl_DrawText(font, txtToUse, pos.x, pos.y, float(textSize), colToUse);

    gsgl_ScissorsStop();
}

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