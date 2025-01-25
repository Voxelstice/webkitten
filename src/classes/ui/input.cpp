#include <windows.h>

#include "ui.h"
#include "../../internal/gsgl/gsgl.h"
#include "../../logger.h"

// NOTE: This is a really rudimentary input function. It does not have selection, clipboard, or any of the fancy features.
void ui_InputBox(GSGL_Font font, const char *def, std::string& buf, int textSize, Vector2i pos, Vector2i size, Color bgColor, Color textColor) {
    const char* txtToUse = def;
    if (buf.length() > 0) txtToUse = buf.c_str();

    bool hovered = gsgl_IsPointInRect(gsgl_GetMousePosition(), pos, size);
    if (hovered == true) {
        char chr = gsgl_GetLastChar();
        if (chr != 0 && chr != 8 && chr >= 39 && chr <= 122 || chr == 32) {
            std::string lastChar = "";
            lastChar = chr;
            buf.append(lastChar);
        } else if (chr == 8 && buf.length() > 0) {
            buf.erase(buf.end() - 1);
        }
    }

    gsgl_ScissorsStart(pos.x, pos.y, size.x, size.y);

    if (bgColor.a != 0) gsgl_Rect(pos.x, pos.y, size.x, size.y, bgColor);
    gsgl_DrawText(GetFont(PROGGY_CLEAN), txtToUse, pos.x, pos.y, float(textSize), textColor);

    gsgl_ScissorsStop();
}