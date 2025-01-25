#include "ui.h"
#include "../../internal/gsgl/gsgl.h"
#include "../../logger.h"

bool ui_BasicButton(GSGL_Font font, const char* txt, int textSize, Vector2i pos, Vector2i size, Color textColor, Color offColor, Color onColor, Color clickColor) {
    bool hovered = gsgl_IsPointInRect(gsgl_GetMousePosition(), pos, size);
    bool held = gsgl_IsMouseButtonDown(GSGL_LMB);
    bool pressed = gsgl_IsMouseButtonReleased(GSGL_LMB);

    gsgl_ScissorsStart(pos.x, pos.y, size.x, size.y);

    gsgl_Rect(pos.x, pos.y, size.x, size.y, hovered == true ? (held == true ? clickColor : onColor) : offColor);

    Vector2i txtSize = gsgl_GetTextSize(font, txt, float(textSize));
    gsgl_DrawText(font, txt, pos.x + (size.x/2) - (txtSize.x/2), pos.y + (size.y/2) - (txtSize.y/2), float(textSize), textColor);

    if (hovered == true) gsgl_SetCursor(GSGL_CLICK);

    gsgl_ScissorsStop();

    return hovered == true && pressed == true;
}