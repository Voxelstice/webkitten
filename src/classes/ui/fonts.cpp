#include "../../internal/gsgl/gsgl.h"
#include "../../logger.h"

// VARIABLES
static GSGL_Font proggyClean;
static GSGL_Font proggyTiny;

typedef enum GameFont {
    PROGGY_CLEAN,
    PROGGY_TINY
} GameFont;

// FUNCTIONS

// some stuff to have in BEFORE anything that uses them
float GetTextSpacing(int fontSize) {
    int defaultFontSize = 12;
    if (fontSize < defaultFontSize) fontSize = defaultFontSize;
    int spacing = fontSize/defaultFontSize;

    return (float)spacing;
}
bool IsFontReady(GSGL_Font font) {
    return gsgl_IsFontValid(font); //&& font.texture.id != 0;
}

void LoadFonts() {
    if (!gsgl_WindowReady()) {
        Logger_log(LOGGER_ERROR, "FONTS: Attempted font registration but the window wasn't ready!");
        return;
    }

    Logger_log(LOGGER_INFO, "FONTS: Registering fonts");

    proggyClean = gsgl_LoadFont("assets/fonts/ProggyClean.ttf");
    if (IsFontReady(proggyClean)) {
        Logger_log(LOGGER_INFO, "FONTS: ProggyClean.ttf loaded");
    } else {
        Logger_log(LOGGER_ERROR, "FONTS: ProggyClean.ttf didn't load!");
        Logger_log(LOGGER_ERROR, "This is a integral font of the application and we don't have a embedded variant at the moment.");
        gsgl_CloseWindow();
    }

    proggyTiny = gsgl_LoadFont("assets/fonts/ProggyTiny.ttf");
    if (IsFontReady(proggyTiny)) {
        Logger_log(LOGGER_INFO, "FONTS: ProggyTiny.ttf loaded");
    } else {
        Logger_log(LOGGER_ERROR, "FONTS: ProggyTiny.ttf didn't load!");
    }

    Logger_log(LOGGER_INFO, "FONTS: Fonts registered");
}
void UnloadFonts() {
    Logger_log(LOGGER_INFO, "FONTS: Unloading fonts");

    if (IsFontReady(proggyClean)) {
        gsgl_UnloadFont(proggyClean);
        Logger_log(LOGGER_INFO, "FONTS: ProggyClean.ttf unloaded");
    }
    if (IsFontReady(proggyTiny)) {
        gsgl_UnloadFont(proggyTiny);
        Logger_log(LOGGER_INFO, "FONTS: ProggyTiny.ttf unloaded");
    }

    Logger_log(LOGGER_INFO, "FONTS: Fonts unloaded");
}
GSGL_Font GetFont(GameFont font) {
    switch (font) {
        case PROGGY_CLEAN:
            return proggyClean;
            break;
        case PROGGY_TINY:
            return proggyTiny;
            break;

        default: {
            if (IsFontReady(proggyClean)) {
                return proggyClean;
            } else {
                return gsgl_InvalidFont();
            }
            break;
        }
    }
}

// Same as DrawText and MeasureText, but with the use of a custom font in one function.
/*void DrawTextFont(Font font, const char *text, Vector2 pos, int fontSize, Color color) {
    if (font.texture.id != 0) {
        int spacing = (int)GetTextSpacing(fontSize);

        DrawTextEx(font, text, pos, (float)fontSize, (float)spacing, color);
    }
}
int MeasureTextFont(Font font, const char *text, int fontSize) {
    Vector2 textSize = { 0.0f, 0.0f };
    if (font.texture.id != 0) {
        int spacing = (int)GetTextSpacing(fontSize);

        textSize = MeasureTextEx(font, text, (float)fontSize, (float)spacing);
    }
    return (int)textSize.x;
}
Vector2 MeasureTextExFont(Font font, const char *text, int fontSize) {
    Vector2 textSize = { 0.0f, 0.0f };
    if (font.texture.id != 0) {
        int defaultFontSize = 10;
        if (fontSize < defaultFontSize) fontSize = defaultFontSize;
        int spacing = (int)GetTextSpacing(fontSize);

        textSize = MeasureTextEx(font, text, (float)fontSize, (float)spacing);
    }
    return textSize;
}*/

/*
// Draw text using font inside rectangle limits with support for text selection
void DrawTextBoxedSelectable(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint, int selectStart, int selectLength, Color selectTint, Color selectBackTint) {
    int length = TextLength(text);  // Total length in bytes of the text, scanned by codepoints in loop

    float textOffsetY = 0;          // Offset between lines (on line break '\n')
    float textOffsetX = 0.0f;       // Offset X to next character to draw

    float scaleFactor = fontSize/(float)font.baseSize;     // Character rectangle scaling factor

    // Word/character wrapping mechanism variables
    enum { MEASURE_STATE = 0, DRAW_STATE = 1 };
    int state = wordWrap? MEASURE_STATE : DRAW_STATE;

    int startLine = -1;         // Index where to begin drawing (where a line begins)
    int endLine = -1;           // Index where to stop drawing (where a line ends)
    int lastk = -1;             // Holds last value of the character position

    for (int i = 0, k = 0; i < length; i++, k++)
    {
        // Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetCodepoint(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol moving one byte
        if (codepoint == 0x3f) codepointByteCount = 1;
        i += (codepointByteCount - 1);

        float glyphWidth = 0;
        if (codepoint != '\n')
        {
            glyphWidth = (font.glyphs[index].advanceX == 0) ? font.recs[index].width*scaleFactor : font.glyphs[index].advanceX*scaleFactor;

            if (i + 1 < length) glyphWidth = glyphWidth + spacing;
        }

        // NOTE: When wordWrap is ON we first measure how much of the text we can draw before going outside of the rec container
        // We store this info in startLine and endLine, then we change states, draw the text between those two variables
        // and change states again and again recursively until the end of the text (or until we get outside of the container).
        // When wordWrap is OFF we don't need the measure state so we go to the drawing state immediately
        // and begin drawing on the next line before we can get outside the container.
        if (state == MEASURE_STATE)
        {
            // TODO: There are multiple types of spaces in UNICODE, maybe it's a good idea to add support for more
            // Ref: http://jkorpela.fi/chars/spaces.html
            if ((codepoint == ' ') || (codepoint == '\t') || (codepoint == '\n')) endLine = i;

            if ((textOffsetX + glyphWidth) > rec.width)
            {
                endLine = (endLine < 1)? i : endLine;
                if (i == endLine) endLine -= codepointByteCount;
                if ((startLine + codepointByteCount) == endLine) endLine = (i - codepointByteCount);

                state = !state;
            }
            else if ((i + 1) == length)
            {
                endLine = i;
                state = !state;
            }
            else if (codepoint == '\n') state = !state;

            if (state == DRAW_STATE)
            {
                textOffsetX = 0;
                i = startLine;
                glyphWidth = 0;

                // Save character position when we switch states
                int tmp = lastk;
                lastk = k - 1;
                k = tmp;
            }
        }
        else
        {
            if (codepoint == '\n')
            {
                if (!wordWrap)
                {
                    textOffsetY += (font.baseSize + font.baseSize/2)*scaleFactor;
                    textOffsetX = 0;
                }
            }
            else
            {
                if (!wordWrap && ((textOffsetX + glyphWidth) > rec.width))
                {
                    textOffsetY += (font.baseSize + font.baseSize/2)*scaleFactor;
                    textOffsetX = 0;
                }

                // When text overflows rectangle height limit, just stop drawing
                if ((textOffsetY + font.baseSize*scaleFactor) > rec.height) break;

                // Draw selection background
                bool isGlyphSelected = false;
                if ((selectStart >= 0) && (k >= selectStart) && (k < (selectStart + selectLength)))
                {
                    DrawRectangleRec({ rec.x + textOffsetX - 1, rec.y + textOffsetY, glyphWidth, (float)font.baseSize*scaleFactor }, selectBackTint);
                    isGlyphSelected = true;
                }

                // Draw current character glyph
                if ((codepoint != ' ') && (codepoint != '\t'))
                {
                    DrawTextCodepoint(font, codepoint, { rec.x + textOffsetX, rec.y + textOffsetY }, fontSize, isGlyphSelected? selectTint : tint);
                }
            }

            if (wordWrap && (i == endLine))
            {
                textOffsetY += (font.baseSize + font.baseSize/2)*scaleFactor;
                textOffsetX = 0;
                startLine = endLine;
                endLine = -1;
                glyphWidth = 0;
                selectStart += lastk - k;
                k = lastk;

                state = !state;
            }
        }

        if ((textOffsetX != 0) || (codepoint != ' ')) textOffsetX += glyphWidth;  // avoid leading spaces
    }
}

void DrawTextBoxed(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint) {
    DrawTextBoxedSelectable(font, text, rec, fontSize, spacing, wordWrap, tint, 0, 0, WHITE, WHITE);
}*/