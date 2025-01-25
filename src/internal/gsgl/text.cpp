// Generic Software Graphics Library (GSGL)
// Designed for software rendering specifically
// Heavily inspired by raylib

// This is the text side of things

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../../logger.h"
#include "gsgl.h"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "libs/stb_truetype.h"

GSGL_Font gsgl_LoadFont(const char* fileName) {
    GSGL_Font font = { 0 };
    
    FILE* file = fopen(fileName, "rb");
    if (!file) {
        Logger_log(LOGGER_ERROR, "GRAPHICS: Could not open font file '%s'.", fileName);
        font.valid = false;
    } else {
        // Read the font file into a buffer
        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        fseek(file, 0, SEEK_SET);
        font.font_buffer = (unsigned char*)malloc(size);
        fread(font.font_buffer, 1, size, file);
        fclose(file);

        // Initialize the font
        if (!stbtt_InitFont(&font.font, font.font_buffer, stbtt_GetFontOffsetForIndex(font.font_buffer, 0))) {
            Logger_log(LOGGER_ERROR, "GRAPHICS: Could not initialize font.");
            free(font.font_buffer);
            font.valid = false;
        } else {
            font.valid = true;
        }
    }

    return font;
}
GSGL_Font gsgl_InvalidFont() {
    GSGL_Font font = { 0 };
    font.valid = false;
    return font;
}
void gsgl_UnloadFont(GSGL_Font font) {
    font.valid = false;
    free(font.font_buffer);
}

Vector2i gsgl_GetCodepointSize(GSGL_Font font, const char* codepoint, float font_size) {
    float scale = stbtt_ScaleForPixelHeight(&font.font, font_size);
    int x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBox(&font.font, (int)*codepoint, scale, scale, &x0, &y0, &x1, &y1);
    return {x1 - x0, y1 - y0};
}
Vector2i gsgl_GetTextSize(GSGL_Font font, const char* text, float font_size) {
    float scale = stbtt_ScaleForPixelHeight(&font.font, font_size);

    int width = 0;
    int height = 0;

    for (const char* p = text; *p; ++p) {
        int codepoint = *p;
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&font.font, codepoint, &advance, &lsb);

        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&font.font, codepoint, scale, scale, &x0, &y0, &x1, &y1);

        int glyph_width = x1 - x0;
        int glyph_height = y1 - y0;

        if (height == 0) height = glyph_height;

        width += (int)(advance * scale);
    }

    return {width, height};
}

void gsgl_DrawText(GSGL_Font font, const char* text, int x, int y, float font_size, Color col) {
    if (font.valid == false) return;

    float scale = stbtt_ScaleForPixelHeight(&font.font, font_size);

    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font.font, &ascent, &descent, &line_gap);
    int baseline = y + (int)(ascent * scale);

    int cursor_x = x;
    int cursor_y = baseline;

    for (const char* p = text; *p; ++p) {
        int codepoint = *p;
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&font.font, codepoint, &advance, &lsb);

        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&font.font, codepoint, scale, scale, &x0, &y0, &x1, &y1);

        int glyph_width = x1 - x0;
        int glyph_height = y1 - y0;

        unsigned char* bitmap = stbtt_GetCodepointBitmap(&font.font, 0, scale, codepoint, &glyph_width, &glyph_height, 0, 0);

        for (int j = 0; j < glyph_height; j++) {
            for (int i = 0; i < glyph_width; i++) {
                int fb_x = cursor_x + i + x0;
                int fb_y = cursor_y + j + y0;

                if (fb_x >= 0 && fb_x < gsgl_GetScreenWidth() && fb_y >= 0 && fb_y < gsgl_GetScreenHeight()) {
                    uint8_t alpha = bitmap[j * glyph_width + i];
                    uint32_t color = (alpha << 24) | (col.r << 16) | (col.g << 8) | col.b;
                    gsgl_BufferAccess(1, fb_y * gsgl_GetScreenWidth() + fb_x, color);
                }
            }
        }

        cursor_x += (int)(advance * scale);

        // do newline aswell
        if (p == "\n") {
            cursor_y += (int)(line_gap * scale);
        }

        stbtt_FreeBitmap(bitmap, NULL);
    }
}

bool gsgl_IsFontValid(GSGL_Font font) {
    return font.valid == true && font.font_buffer != NULL;
}