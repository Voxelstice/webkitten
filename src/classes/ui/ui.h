#pragma once

// General UI header
// Just includes everything it can
// and defines some stuff

#include <string>

#include "../../internal/gsgl/gsgl.h"
#include "fonts.h"

#include "classes/input.h"

typedef struct ui_InputData {
    std::string *curString;

    int selection;
    int selectionLength;
} ui_InputData;

bool ui_BasicButton(GSGL_Font font, const char* txt, int textSize, Vector2i pos, Vector2i size, Color textColor, Color offColor, Color onColor, Color clickColor);
void ui_InputBox(GSGL_Font font, const char* def, std::string& buf, int textSize, Vector2i pos, Vector2i size, Color bgColor, Color textColor);