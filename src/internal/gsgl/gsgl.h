// Generic Software Graphics Library (GSGL)
// Designed for software rendering specifically
// Heavily inspired by raylib

// More info about library design is in graphics.cpp

#pragma once

#ifndef GSGL_API
    #define GSGL_API
#endif

#include <cstdint>

#include "libs/stb_truetype.h"

// == TYPES

// some types start with GSGL, some dont.
// this is just for the sake of typing out less of very common components

typedef struct Vector2 {
    float x;
    float y;
} Vector2;
typedef struct Vector2i {
    int x;
    int y;
} Vector2i;

typedef struct Color {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} Color;

typedef struct GSGL_Texture {

} GSGL_Texture;

typedef struct GSGL_Font {
    stbtt_fontinfo font;
    unsigned char* font_buffer;

    bool valid;
} GSGL_Font;

// input related
typedef enum {
    GSGL_LMB = 0,
    GSGL_RMB = 1,
    GSGL_MMB = 2,
} GSGL_MouseButton;
typedef struct GSGL_MouseInputObject {
    bool leftMouseButton;
    bool middleMouseButton;
    bool rightMouseButton;
    int mouseScrollWheel;

    int mouseX;
    int mouseY;
} GSGL_MouseInputObject;
typedef enum {
    KEY_NULL            = 0,        // Key: NULL, used for no key pressed

    KEY_APOSTROPHE      = 39,       // Key: '
    KEY_COMMA           = 44,       // Key: ,
    KEY_MINUS           = 45,       // Key: -
    KEY_PERIOD          = 46,       // Key: .
    KEY_SLASH           = 47,       // Key: /
    KEY_ZERO            = 48,       // Key: 0
    KEY_ONE             = 49,       // Key: 1
    KEY_TWO             = 50,       // Key: 2
    KEY_THREE           = 51,       // Key: 3
    KEY_FOUR            = 52,       // Key: 4
    KEY_FIVE            = 53,       // Key: 5
    KEY_SIX             = 54,       // Key: 6
    KEY_SEVEN           = 55,       // Key: 7
    KEY_EIGHT           = 56,       // Key: 8
    KEY_NINE            = 57,       // Key: 9
    KEY_SEMICOLON       = 59,       // Key: ;
    KEY_EQUAL           = 61,       // Key: =
    KEY_A               = 65,       // Key: A | a
    KEY_B               = 66,       // Key: B | b
    KEY_C               = 67,       // Key: C | c
    KEY_D               = 68,       // Key: D | d
    KEY_E               = 69,       // Key: E | e
    KEY_F               = 70,       // Key: F | f
    KEY_G               = 71,       // Key: G | g
    KEY_H               = 72,       // Key: H | h
    KEY_I               = 73,       // Key: I | i
    KEY_J               = 74,       // Key: J | j
    KEY_K               = 75,       // Key: K | k
    KEY_L               = 76,       // Key: L | l
    KEY_M               = 77,       // Key: M | m
    KEY_N               = 78,       // Key: N | n
    KEY_O               = 79,       // Key: O | o
    KEY_P               = 80,       // Key: P | p
    KEY_Q               = 81,       // Key: Q | q
    KEY_R               = 82,       // Key: R | r
    KEY_S               = 83,       // Key: S | s
    KEY_T               = 84,       // Key: T | t
    KEY_U               = 85,       // Key: U | u
    KEY_V               = 86,       // Key: V | v
    KEY_W               = 87,       // Key: W | w
    KEY_X               = 88,       // Key: X | x
    KEY_Y               = 89,       // Key: Y | y
    KEY_Z               = 90,       // Key: Z | z
    KEY_LEFT_BRACKET    = 91,       // Key: [
    KEY_BACKSLASH       = 92,       // Key: '\'
    KEY_RIGHT_BRACKET   = 93,       // Key: ]
    KEY_GRAVE           = 96,       // Key: `

    KEY_SPACE           = 32,       // Key: Space
    KEY_ESCAPE          = 256,      // Key: Esc
    KEY_ENTER           = 257,      // Key: Enter
    KEY_TAB             = 258,      // Key: Tab
    KEY_BACKSPACE       = 259,      // Key: Backspace
    KEY_INSERT          = 260,      // Key: Ins
    KEY_DELETE          = 261,      // Key: Del
    KEY_RIGHT           = 262,      // Key: Cursor right
    KEY_LEFT            = 263,      // Key: Cursor left
    KEY_DOWN            = 264,      // Key: Cursor down
    KEY_UP              = 265,      // Key: Cursor up
    KEY_PAGE_UP         = 266,      // Key: Page up
    KEY_PAGE_DOWN       = 267,      // Key: Page down
    KEY_HOME            = 268,      // Key: Home
    KEY_END             = 269,      // Key: End
    KEY_CAPS_LOCK       = 280,      // Key: Caps lock
    KEY_SCROLL_LOCK     = 281,      // Key: Scroll down
    KEY_NUM_LOCK        = 282,      // Key: Num lock
    KEY_PRINT_SCREEN    = 283,      // Key: Print screen
    KEY_PAUSE           = 284,      // Key: Pause
    KEY_F1              = 290,      // Key: F1
    KEY_F2              = 291,      // Key: F2
    KEY_F3              = 292,      // Key: F3
    KEY_F4              = 293,      // Key: F4
    KEY_F5              = 294,      // Key: F5
    KEY_F6              = 295,      // Key: F6
    KEY_F7              = 296,      // Key: F7
    KEY_F8              = 297,      // Key: F8
    KEY_F9              = 298,      // Key: F9
    KEY_F10             = 299,      // Key: F10
    KEY_F11             = 300,      // Key: F11
    KEY_F12             = 301,      // Key: F12
    KEY_LEFT_SHIFT      = 340,      // Key: Shift left
    KEY_LEFT_CONTROL    = 341,      // Key: Control left
    KEY_LEFT_ALT        = 342,      // Key: Alt left
    KEY_LEFT_SUPER      = 343,      // Key: Super left
    KEY_RIGHT_SHIFT     = 344,      // Key: Shift right
    KEY_RIGHT_CONTROL   = 345,      // Key: Control right
    KEY_RIGHT_ALT       = 346,      // Key: Alt right
    KEY_RIGHT_SUPER     = 347,      // Key: Super right
    KEY_KB_MENU         = 348,      // Key: KB menu
} GSGL_Key;

// == CORE GRAPHICS

// modes
GSGL_API void gsgl_SoftwareRender(); // Sets renderer mode to software.
GSGL_API void gsgl_HardwareRender(); // Sets renderer mode to hardware-accelerated.

// main stuff
GSGL_API void gsgl_InitWindow(int width, int height, const char *title); // Initializes the library and the window.
GSGL_API void gsgl_CloseWindow(); // Closes the window.
GSGL_API bool gsgl_WindowReady(); // Returns if the window is ready.

GSGL_API void gsgl_SetFrameRate(int framerate); // Sets the internal frame limiter.
GSGL_API void gsgl_PollEvents(); // Polls events.
GSGL_API bool gsgl_ShouldClose(); // Returns if the window is closing.

GSGL_API void gsgl_GetLastError(); // Gets last Windows error

GSGL_API int gsgl_GetScreenWidth(); // Gets screen width
GSGL_API int gsgl_GetScreenHeight(); // Gets screen height

GSGL_API bool gsgl_IsWindowMinimized();
GSGL_API bool gsgl_IsWindowMaximized();

// rudimentary graphics
GSGL_API void gsgl_Draw(); // Draws the on-screen buffer onto the screen
GSGL_API void gsgl_SwapBuffers(); // Swaps the buffers and resets the off-screen buffer
GSGL_API void gsgl_Pixel(int x, int y, Color col); // Draws a pixel.
GSGL_API void gsgl_Rect(int x, int y, int width, int height, Color col); // Draws a rectangle.
GSGL_API void gsgl_RectOutline(int x, int y, int width, int height, int thickness, Color col); // Draws a rectangle outline.
GSGL_API void gsgl_Clear(Color col); // Sets the buffer clear color.

GSGL_API void gsgl_BufferAccess(int buffer, int index, uint32_t color); // Sets a value inside the buffer directly. Use if you know what you're doing.

// scissors
GSGL_API void gsgl_ScissorsStart(int x, int y, int width, int height); // Starts "scissors". Clips any further pixels that will get drawn into a rect.
GSGL_API void gsgl_ScissorsStop(); // Stops "scissors".

// write box
GSGL_API void gsgl_WriteBoxReset();
GSGL_API void gsgl_WriteBoxSet(int x, int y, int width, int height);
GSGL_API void gsgl_WriteBoxUpdate(int x, int y);

// some utils
GSGL_API float gsgl_Lerp(float start, float end, float amount); // Interpolates a number.
GSGL_API uint32_t gsgl_HandleAlpha(uint32_t a, uint32_t b); // Handles alpha blending between 2 colors.
GSGL_API Color gsgl_UnpackColor(uint32_t col); // Unpacks a uint32 color to GSGL_Color.
GSGL_API uint32_t gsgl_PackColor(Color col); // Packs a color struct into a uint32_t.

// timer
GSGL_API void gsgl_InitTimer();
GSGL_API double gsgl_GetTime();
GSGL_API void gsgl_WaitTime(double seconds);

// == INPUT
// handled in the core too

// mouse
GSGL_API Vector2i gsgl_GetMousePosition(); // Gets current mouse position

GSGL_API bool gsgl_IsMouseButtonDown(GSGL_MouseButton button); // Returns true if the mouse button is down
GSGL_API bool gsgl_IsMouseButtonUp(GSGL_MouseButton button); // Returns true if the mouse button is up

GSGL_API bool gsgl_IsMouseButtonPressed(GSGL_MouseButton button); // Returns true if the mouse button is pressed
GSGL_API bool gsgl_IsMouseButtonReleased(GSGL_MouseButton button); // Returns true if the mouse button is released

// keyboard
GSGL_API bool gsgl_IsKeyDown(GSGL_Key key); // Returns true if specific key is down
GSGL_API bool gsgl_IsKeyUp(GSGL_Key key); // Returns true if specific key is up

GSGL_API bool gsgl_IsKeyPressed(GSGL_Key key); // Returns true if specific key is pressed
GSGL_API bool gsgl_IsKeyReleased(GSGL_Key key); // Returns true if specific key is released

GSGL_API char gsgl_GetLastChar(); // Returns the last character

// == TEXT & FONTS
GSGL_API GSGL_Font gsgl_LoadFont(const char* fileName); // Loads a font.
GSGL_API GSGL_Font gsgl_InvalidFont(); // Returns a invalid font.
GSGL_API void gsgl_UnloadFont(GSGL_Font font); // Unloads a font.
GSGL_API void gsgl_DrawText(GSGL_Font font, const char* text, int x, int y, float font_size, Color col); // Draws text.
GSGL_API bool gsgl_IsFontValid(GSGL_Font font); // Returns true if the font is valid

GSGL_API Vector2i gsgl_GetCodepointSize(GSGL_Font font, const char *codepoint, float font_size);
GSGL_API Vector2i gsgl_GetTextSize(GSGL_Font font, const char *text, float font_size);

// == UTILS
GSGL_API bool gsgl_IsPointInRect(Vector2i point, Vector2i pos, Vector2i size);