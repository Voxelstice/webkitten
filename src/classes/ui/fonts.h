#pragma once

#include "../../internal/gsgl/gsgl.h"
#include <string>

typedef enum GameFont {
    PROGGY_CLEAN,
    PROGGY_TINY
} GameFont;

bool IsFontReady(GSGL_Font font);

void LoadFonts();
void UnloadFonts();
GSGL_Font GetFont(GameFont font);

/*void DrawTextFont(Font font, const char *text, Vector2 pos, int fontSize, Color color);
int MeasureTextFont(Font font, const char *text, int fontSize);
Vector2 MeasureTextExFont(Font font, const char *text, int fontSize);
float GetTextSpacing(int fontSize);*/

//void DrawTextBoxed(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint);
//void DrawTextBoxedSelectable(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint, int selectStart, int selectLength, Color selectTint, Color selectBackTint);