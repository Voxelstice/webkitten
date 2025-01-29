// Generic Software Graphics Library (GSGL)
// Designed for software rendering specifically
// Heavily inspired by raylib

/*

library design woes

If you plan on using this, maybe don't. This was custom-made for this browser specifically due to the need of software rendering
and hardware rendering support. This also may not be practical in any other applications and may be unstable and make undesired behavior

But if you still want to, here's a quick bouncing ball example.

#include "gsgl/gsgl.h"

int main() {
    gsgl_SoftwareRender();
    gsgl_InitWindow(1600, 900, "bouncing ball");
    gsgl_SetFrameRate(60);

    int gooberX = 0;
    int gooberY = 0;

    int speed = 2;
    int gooberVX = speed;
    int gooberVY = speed;

    while (!gsgl_ShouldClose()) {
        gsgl_PollEvents();

        if (gooberX >= gsgl_GetScreenWidth()-20) {
            gooberVX = -speed;
        } else if (gooberX <= 0) {
            gooberVX = speed;
        }
        if (gooberY >= gsgl_GetScreenHeight()-20) {
            gooberVY = -speed;
        } else if (gooberY <= 0) {
            gooberVY = speed;
        }

        gooberX += gooberVX;
        gooberY += gooberVY;

        gsgl_Rect(gooberX, gooberY, 20, 20, {255, 255, 255, 255});

        gsgl_Draw();
        gsgl_Clear({32, 32, 32, 255});
        gsgl_SwapBuffers();
    }

    gsgl_CloseWindow();

    return 0;
}

Now here's a couple of quirks:
- The clear comes specifically before SwapBuffers for one reason: it's done in SwapBuffers. What does that function do then? Set the clear color.
  SwapBuffers sets the off-screen buffer to the clear color, which by default is BLACK unless you set the clear color.
  It'll still work if you put it anywhere else, but this is more reliable.

  This is not in effect if you're using a hardware renderer

- GLFW is never, ever used. This may cause cross-platform problems, ESPECIALLY ON MACOS AND LINUX!!!
  While I actually took some effort making it work for Linux (my original intention with this entire web browser project was to make a lightweight
  browser that could run smoothly on my Futijsu P1510 Lifebook. no, Pale Moon isn't smooth enough.)
  The only thing that would ever be an actual issue is MacOS. I don't have a modern Mac device that has that fancy Metal stuff.

- (plans) OpenGL ES is going to be used for hardware acceleration.

Optimizations:
- The clear thing. I've mentioned it above.

- Normally doing SwapBuffers would do it for ALL pixels. This is not very performant. So we're gonna use a box.
  This box is basically like the Scissors mode. Expect its set when you do BufferAccess. It'll do comparing, and it'll set if it needs to.
  Changing the clear color also forces the box to the screen's resolution.
  Internally, this is called WriteBox. If you need to manipulate this in some way, there's a WriteBoxSet function.
  BufferAccess also calls WriteBoxUpdate.

  This may seem like a pointless optimization, but even on modern systems at most it's a 5% CPU usage reduction.
  Such reduction may be very benefical in older hardware.

  This is not in effect if you're using a hardware renderer

Some things to keep in mind:
- All functions start with "gsgl_". This is to prevent conflict with windows.h specifically
- The renderer mode must be set BEFORE gsgl_InitWindow.

*/

#include <cstdint>
#include <chrono>
#include <time.h>
#include <comdef.h>
#include <windows.h>
#include <windowsx.h>

#include "../../logger.h"
#include "gsgl.h"

#define SUPPORT_WINMM_HIGHRES_TIMER      1
#define SUPPORT_PARTIALBUSY_WAIT_LOOP    1
#define KEYBOARD_KEYS                    512

typedef struct GraphicsCore {
    HINSTANCE instance;
    bool ready;
    struct {
        // windows stuff
        HWND handle;
        MSG msg;
        HCURSOR cursor;

        // our stuff
        int width;
        int height;

        bool closing;
    } Window;
    struct {
        // settings
        bool software;
        bool hardware;
        int frameRate;

        // software rendering
        bool buffersInited;
        uint32_t* buffer1; // off-screen
        uint32_t* buffer2; // on-screen

        uint32_t swapBufferClear;

        // hardware rendering

        // extra
        struct {
            bool active;

            int startX;
            int startY;
            int endX;
            int endY;
        } Scissors;

        // optimization. software renderer only
        struct {
            int startX;
            int startY;
            int endX;
            int endY;
        } WriteBox;
    } Graphics;

    struct {
        GSGL_MouseInputObject mouseNew;
        GSGL_MouseInputObject mouseOld;

        bool keysNew[KEYBOARD_KEYS];
        bool keysOld[KEYBOARD_KEYS];
        int keysRepeat[KEYBOARD_KEYS];

        char lastChar;

        int repeatDelay;
        int repeatSpeed;

        GSGL_Cursor cursorStyle;
        bool cursorChanged;
    } Input;

    struct { // This part is directly taken from raylib
        double current;                     // Current time measure
        double previous;                    // Previous time measure
        double update;                      // Time measure for frame update
        double draw;                        // Time measure for frame draw
        double frame;                       // Time measure for one frame
        double target;                      // Desired time for one frame, if 0 not applied
        unsigned long long int base;
        unsigned int frameCounter;          // Frame counter
    } Time;
} GraphicsCore;

GraphicsCore core = { 0 };

// Internal functions
void gi_UpdateSettings();
void gi_ResizeWindow(int width, int height);
void gi_InitBuffers();

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void gsgl_GetLastError() { 
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();

    if (FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL) == 0)
    {
        Logger_log(LOGGER_ERROR, "GRAPHICS: Internal error: FormatMessage failed", lpMsgBuf);
        MessageBox(NULL, "FormatMessage failed", "tinyweb", MB_OK);
        gsgl_CloseWindow();
    }

    Logger_log(LOGGER_ERROR, "GRAPHICS: Internal error: %s", lpMsgBuf);
    MessageBox(NULL, (LPCSTR)lpMsgBuf, "tinyweb", MB_OK);

    LocalFree(lpMsgBuf);
    gsgl_CloseWindow();
    
    //ExitProcess(dw); 
}

// Core library functions
void gsgl_SoftwareRender() {
    core.Graphics.software = true;
    core.Graphics.hardware = false;
}
void gsgl_HardwareRender() {
    core.Graphics.software = false;
    core.Graphics.hardware = true;
}

void gsgl_InitWindow(int width, int height, const char *title) {
    Logger_log(LOGGER_INFO, "GRAPHICS: Initializing internal graphics library");

    // initialize core
    core.instance = GetModuleHandle(0);
    core.ready = false;

    core.Window.msg = { };
    core.Window.closing = false;

    core.Graphics.frameRate = 60;

    core.Graphics.Scissors.active = false;
    core.Graphics.Scissors.startX = 0;
    core.Graphics.Scissors.startY = 0;
    core.Graphics.Scissors.endX = 0;
    core.Graphics.Scissors.endY = 0;

    core.Input.mouseOld = { 0 };
    core.Input.mouseNew = { 0 };

    for (int i = 0; i < KEYBOARD_KEYS; i++) {
        core.Input.keysOld[i] = false;
        core.Input.keysNew[i] = false;
        core.Input.keysRepeat[i] = 0;
    }

    core.Input.lastChar = 0;
    core.Input.cursorStyle = GSGL_POINTER;
    core.Input.cursorChanged = true;

    gi_UpdateSettings();

    // set class  name
    const char* className = "tinyweb";

    // register class
    WNDCLASS winClass = { };

    winClass.lpfnWndProc = WindowProc;
    winClass.hInstance = core.instance;
    winClass.lpszClassName = className;

    Logger_log(LOGGER_INFO, "GRAPHICS: - Registering class");
    RegisterClass(&winClass);

    // create window
    Logger_log(LOGGER_INFO, "GRAPHICS: - Creating window");
    core.Window.handle = CreateWindowEx(
        0, className, "tinyweb", WS_OVERLAPPEDWINDOW,
        0, 0, width, height,
        NULL, NULL, core.instance, NULL
    );

    if (core.Window.handle == NULL) {
        gsgl_GetLastError();
        return;
    } else {
        Logger_log(LOGGER_INFO, "GRAPHICS: - Window successfully created");
    }

    // show it
    Logger_log(LOGGER_INFO, "GRAPHICS: - Showing window");
    ShowWindow(core.Window.handle, SW_NORMAL);

    // set cursor
    core.Window.cursor = LoadCursor(NULL, IDC_ARROW);
    SetCursor(core.Window.cursor);

    // initialize framebuffers if needed
    core.Window.width = width;
    core.Window.height = height;

    core.Graphics.buffersInited = false;

    if (core.Graphics.software == true) {
        Logger_log(LOGGER_INFO, "GRAPHICS: Starting software rendering initialization");

        // so we're gonna play some tricks here.
        // rather than be expensive and loop through the buffer during draw time again,
        // we're just gonna set the off-screen buffer to the clear color the developer sets.
        // clever right?

        // there may be some reasons as to why this is probably not preferred, but i take this over a few cpu cycles wasted.
        // it may be also just one damn loop extra. dont get me started on having 2 extra loops.

        core.Graphics.swapBufferClear = 0;

        Logger_log(LOGGER_INFO, "GRAPHICS: - Initializing framebuffers");
        gi_InitBuffers();

        Logger_log(LOGGER_INFO, "GRAPHICS: Software renderer prepared");
    } else if (core.Graphics.hardware == true) {
        Logger_log(LOGGER_INFO, "GRAPHICS: Starting hardware-accelerated rendering initialization");

        throw "No hardware acceleration support!";

        Logger_log(LOGGER_INFO, "GRAPHICS: Hardware-accelerated renderer prepared");
    }

    // initiate timer
    gsgl_InitTimer();

    Logger_log(LOGGER_INFO, "GRAPHICS: Internal graphics library initialized");

    core.ready = true;
}

bool gsgl_WindowReady() {
    // a series of checks
    // a lot of this may be unnecessary

    if (core.ready == true) {
        if (core.Graphics.software == true || core.Graphics.hardware == true) {
            if (core.Graphics.buffersInited == true) {
                return true;
            } else {
                Logger_log(LOGGER_ERROR, "GRAPHICS: Attempted to do graphics operation but window wasn't ready (BuffersNotReady)");
                throw "Buffers not ready";
            }
        } else {
            Logger_log(LOGGER_ERROR, "GRAPHICS: Attempted to do graphics operation but window wasn't ready (RendererNotSet)");
            throw "Renderer wasn't set";
        }
    } else {
        Logger_log(LOGGER_ERROR, "GRAPHICS: Attempted to do graphics operation but window wasn't ready (CoreNotReady)");
        throw "Core is not ready";
    }
    return false;
}

void gsgl_SetFrameRate(int framerate) {
    core.Graphics.frameRate = framerate;

    if (framerate < 1) core.Time.target = 0.0f;
    else core.Time.target = 1.0f / double(framerate);
}

void gsgl_PollEvents() {
    gsgl_WindowReady();

    // reset input states
    core.Input.lastChar = 0;
    core.Input.mouseOld = core.Input.mouseNew;

    for (int i = 0; i < KEYBOARD_KEYS; i++) {
        core.Input.keysOld[i] = core.Input.keysNew[i];
    }
    
    int res = GetMessage(&core.Window.msg, core.Window.handle, 0, 0);
    if (res == 0) {
        gsgl_CloseWindow();
    } else {
        TranslateMessage(&core.Window.msg);
        DispatchMessage(&core.Window.msg);
    }
}

bool gsgl_ShouldClose() {
    return core.Window.closing;
}

void gsgl_CloseWindow() {
    core.Window.closing = true;

    PostQuitMessage(0);
}

int gsgl_GetScreenWidth() {
    return core.Window.width;
}
int gsgl_GetScreenHeight() {
    return core.Window.height;
}

bool gsgl_IsWindowMinimized() {
    return IsIconic(core.Window.handle);
}
bool gsgl_IsWindowMaximized() {
    return IsZoomed(core.Window.handle);
}
bool gsgl_IsWindowVisible() {
    // Currently functions like IsWindowMinimized.
    // This is just future proofing when I need it at one point.
    return IsIconic(core.Window.handle);
}

// Graphics functions
void gsgl_Draw() {
    if (gsgl_IsWindowVisible()) return;

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = core.Window.width;
    bmi.bmiHeader.biHeight = -core.Window.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(core.Window.handle);
    StretchDIBits(hdc, 0, 0, core.Window.width, core.Window.height, 0, 0, core.Window.width, core.Window.height, core.Graphics.buffer2, &bmi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(core.Window.handle, hdc);
}
void gsgl_SwapBuffers() {
    gsgl_WindowReady();

    int start = core.Graphics.WriteBox.startX * core.Graphics.WriteBox.startY;
    int end = core.Graphics.WriteBox.endX * core.Graphics.WriteBox.endY;

    for (int i = start; i < end; i++) {
        core.Graphics.buffer2[i] = core.Graphics.buffer1[i];
        core.Graphics.buffer1[i] = core.Graphics.swapBufferClear;
    }

    gsgl_WriteBoxReset();

    // Update cursor
    if (core.Input.cursorChanged == true && !gsgl_IsWindowVisible()) {
        // This is a bit messy.
        if (core.Input.cursorStyle == GSGL_POINTER) {
            core.Window.cursor = LoadCursor(NULL, IDC_ARROW);
        } else if (core.Input.cursorStyle == GSGL_CLICK) {
            core.Window.cursor = LoadCursor(NULL, IDC_HAND);
        } else if (core.Input.cursorStyle == GSGL_TEXT) {
            core.Window.cursor = LoadCursor(NULL, IDC_IBEAM);
        }
        SetCursor(core.Window.cursor);

        core.Input.cursorChanged = false;
    }
    if (core.Input.cursorStyle != GSGL_POINTER) {
        gsgl_SetCursor(GSGL_POINTER);
    }

    // Framerate limiter
    core.Time.current = gsgl_GetTime();
    core.Time.draw = core.Time.current - core.Time.previous;
    core.Time.previous = core.Time.current;

    core.Time.frame = core.Time.update + core.Time.draw;

    // Wait for some milliseconds...
    if (core.Time.frame < core.Time.target) {
        gsgl_WaitTime(core.Time.target - core.Time.frame);

        core.Time.current = gsgl_GetTime();
        double waitTime = core.Time.current - core.Time.previous;
        core.Time.previous = core.Time.current;

        core.Time.frame += waitTime; // Total frame time: update + draw + wait
    }
}

void gsgl_BufferAccess(int buffer, int index, uint32_t color) {
    if (gsgl_IsWindowVisible()) return; // optimization

    int x = (int)floor(index % core.Window.width);
    int y = (int)floor(index / core.Window.width);
    if (core.Graphics.Scissors.active == true) {
        if ((x < core.Graphics.Scissors.startX || x >= core.Graphics.Scissors.endX) || (y < core.Graphics.Scissors.startY || y >= core.Graphics.Scissors.endY)) {
            return;
        }
    }

    gsgl_WriteBoxUpdate(x, y);

    if (buffer == 1) {
        core.Graphics.buffer1[index] = gsgl_HandleAlpha(core.Graphics.buffer1[index], color);
    } else if (buffer == 2) {
        core.Graphics.buffer2[index] = gsgl_HandleAlpha(core.Graphics.buffer2[index], color);
    }
}

void gsgl_Pixel(int x, int y, Color col) {
    uint32_t color = gsgl_PackColor(col);

    if (x < 0) return;
    if (y < 0) return;
    if (x >= core.Window.width) return;
    if (y >= core.Window.height) return;

    gsgl_BufferAccess(1, y * core.Window.width + x, color);
}
void gsgl_Rect(int x, int y, int width, int height, Color col) {
    uint32_t color = gsgl_PackColor(col);

    for (int j = y; j < y + height; j++) {
        if (j < 0) continue;
        if (j >= core.Window.height) continue;
        for (int i = x; i < x + width; i++) {
            if (i < 0) continue;
            if (i >= core.Window.width) continue;
            gsgl_BufferAccess(1, j * core.Window.width + i, color);
        }
    }
}
void gsgl_RectOutline(int x, int y, int width, int height, int thickness, Color col) {
    uint32_t color = gsgl_PackColor(col);

    for (int j = y; j < y + height; j++) {
        if (j < 0) continue;
        if (j >= core.Window.height) continue;
        for (int i = x; i < x + width; i++) {
            if (i < 0) continue;
            if (i >= core.Window.width) continue;

            if ((j-y < thickness || j-y >= height-thickness) || (i-x < thickness || i-x >= width-thickness))
                gsgl_BufferAccess(1, j * core.Window.width + i, color);
        }
    }
}

void gsgl_Clear(Color color) {
    if (core.Graphics.swapBufferClear != gsgl_PackColor(color)) {
        core.Graphics.swapBufferClear = gsgl_PackColor(color);
        gsgl_WriteBoxSet(0, 0, core.Window.width, core.Window.height);
    }
}

// Scissors
void gsgl_ScissorsStart(int x, int y, int width, int height) {
    core.Graphics.Scissors.active = true;
    core.Graphics.Scissors.startX = x;
    core.Graphics.Scissors.startY = y;
    core.Graphics.Scissors.endX = x + width;
    core.Graphics.Scissors.endY = y + height;
}
void gsgl_ScissorsStop() {
    core.Graphics.Scissors.active = false;
}

// Write box
void gsgl_WriteBoxReset() {
    core.Graphics.WriteBox.startX = 0;
    core.Graphics.WriteBox.startY = 0;
    core.Graphics.WriteBox.endX = 0;
    core.Graphics.WriteBox.endY = 0;
}
void gsgl_WriteBoxSet(int x, int y, int width, int height) {
    core.Graphics.WriteBox.startX = x;
    core.Graphics.WriteBox.startY = y;
    core.Graphics.WriteBox.endX = x + width;
    core.Graphics.WriteBox.endY = y + height;
}
void gsgl_WriteBoxUpdate(int x, int y) {
    if (x < core.Graphics.WriteBox.startX) core.Graphics.WriteBox.startX = x;
    if (x > core.Graphics.WriteBox.endX) core.Graphics.WriteBox.endX = x;

    if (y < core.Graphics.WriteBox.startY) core.Graphics.WriteBox.startY = y;
    if (y > core.Graphics.WriteBox.endY) core.Graphics.WriteBox.endY = y;

    // cappings
    if (core.Graphics.WriteBox.startX < 0) core.Graphics.WriteBox.startX = 0;
    if (core.Graphics.WriteBox.startY < 0) core.Graphics.WriteBox.startY = 0;
    if (core.Graphics.WriteBox.startX >= core.Window.width) core.Graphics.WriteBox.startX = core.Window.width;
    if (core.Graphics.WriteBox.startY >= core.Window.height) core.Graphics.WriteBox.startY = core.Window.height;

    if (core.Graphics.WriteBox.endX < 0) core.Graphics.WriteBox.endX = 0;
    if (core.Graphics.WriteBox.endY < 0) core.Graphics.WriteBox.endY = 0;
    if (core.Graphics.WriteBox.endX >= core.Window.width) core.Graphics.WriteBox.endX = core.Window.width;
    if (core.Graphics.WriteBox.endY >= core.Window.height) core.Graphics.WriteBox.endY = core.Window.height;
}

// Utility functions
float gsgl_Lerp(float start, float end, float amount) {
    float result = start + amount * (end - start);
    return result;
}
#pragma runtime_checks("", off)
uint32_t gsgl_HandleAlpha(uint32_t a, uint32_t b) {
    // TODO: consider the first alpha maybe
    float a1 = (float)((a >> 24) & 0xFF);
    float a2 = (float)((b >> 24) & 0xFF);

    // do a quick check if we actually need to do this.
    if (a2 > 0 && a2 < 255) {
        Color col1 = gsgl_UnpackColor(a);
        Color col2 = gsgl_UnpackColor(b);

        float r1 = (float)col1.r;
        float g1 = (float)col1.g;
        float b1 = (float)col1.b;

        float r2 = (float)col2.r;
        float g2 = (float)col2.g;
        float b2 = (float)col2.b;

        float amount = a2/255;
        float r3 = gsgl_Lerp(r1, r2, amount);
        float g3 = gsgl_Lerp(g1, g2, amount);
        float b3 = gsgl_Lerp(b1, b2, amount);
        float a3 = a2; // stays as it is. might change. just future-proofing

        return gsgl_PackColor({ (unsigned char)r3, (unsigned char)g3, (unsigned char)b3, (unsigned char)a3 });
    } else if (a2 >= 255) {
        return b;
    } else if (a2 <= 0) {
        return a;
    } else {
        return a;
    }
}
#pragma runtime_checks("", restore)
Color gsgl_UnpackColor(uint32_t col) {
    Color color = { 0 };

    color.a = (unsigned char)(col >> 24) & 0xFF;
    color.r = (unsigned char)(col >> 16) & 0xFF;
    color.g = (unsigned char)(col >> 8) & 0xFF;
    color.b = (unsigned char) col & 0xFF;

    return color;
}
uint32_t gsgl_PackColor(Color col) {
    return (col.a << 24) | (col.r << 16) | (col.g << 8) | col.b;
}

// Internal functions
void gi_UpdateSettings() {
    // update some stuff. not a lot needs to be updated here currently
    // this seems to fire somewhat frequently but its kind of weird when it updates

    DWORD repeatDelay;
    DWORD repeatSpeed;

    SystemParametersInfoA(SPI_GETKEYBOARDDELAY, 0, &repeatDelay, 0);
    SystemParametersInfoA(SPI_GETKEYBOARDSPEED, 0, &repeatSpeed, 0);

    core.Input.repeatDelay = int(float(repeatDelay) * 60);
    core.Input.repeatSpeed = int(roundf((1 / float(repeatSpeed+1)) * 60));
}
void gi_ResizeWindow(int width, int height) {
    core.Window.width = width;
    core.Window.height = height;

    gi_InitBuffers();
}
void gi_InitBuffers() {
    if (core.Graphics.software == true) {
        if (core.Graphics.buffersInited == true) free(core.Graphics.buffer1);
        core.Graphics.buffer1 = (uint32_t*) malloc(core.Window.width * core.Window.height * sizeof(uint32_t));
        memset(core.Graphics.buffer1, 0, core.Window.width * core.Window.height * sizeof(uint32_t));

        if (core.Graphics.buffersInited == true) free(core.Graphics.buffer2);
        core.Graphics.buffer2 = (uint32_t*) malloc(core.Window.width * core.Window.height * sizeof(uint32_t));
        memset(core.Graphics.buffer2, 0, core.Window.width * core.Window.height * sizeof(uint32_t));

        if (core.Graphics.buffersInited == false)
            core.Graphics.buffersInited = true;
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // center the window
            RECT rc;
            GetWindowRect(hwnd, &rc);

            int xPos = (GetSystemMetrics(SM_CXSCREEN) - rc.right)/2;
            int yPos = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom)/2;

            SetWindowPos(hwnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
            return 0;
        }
        case WM_DESTROY: {
            gsgl_CloseWindow();
            return 0;
        }
        case WM_PAINT: {
            // we have to return 0 on this one.
            // otherwise it's not gonna trigger any updates
            return 0;
        }
        case WM_SIZE: {
            int width = LOWORD(lParam);  // Macro to get the low-order word.
            int height = HIWORD(lParam); // Macro to get the high-order word.

            gi_ResizeWindow(width, height);
            return 0;
        }

        // input
        // mouse
        case WM_LBUTTONDOWN: {
            core.Input.mouseNew.leftMouseButton = true;
            return 0;
        }
        case WM_LBUTTONUP: {
            core.Input.mouseNew.leftMouseButton = false;
            return 0;
        }

        case WM_MBUTTONDOWN: {
            core.Input.mouseNew.middleMouseButton = true;
            return 0;
        }
        case WM_MBUTTONUP: {
            core.Input.mouseNew.middleMouseButton = false;
            return 0;
        }

        case WM_RBUTTONDOWN: {
            core.Input.mouseNew.rightMouseButton = true;
            return 0;
        }
        case WM_RBUTTONUP: {
            core.Input.mouseNew.rightMouseButton = false;
            return 0;
        }

        case WM_MOUSEMOVE: {
            int xPos = GET_X_LPARAM(lParam); 
            int yPos = GET_Y_LPARAM(lParam);
            core.Input.mouseNew.mouseX = xPos;
            core.Input.mouseNew.mouseY = yPos;
            return 0;
        }

        // keyboard
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
            // whenever i want to add any more keys into the big enum thing i just use this
            //printf("key down: %i\n", int(wParam));

            core.Input.keysNew[(int)wParam] = true;
            return 0;
        }
        case WM_SYSKEYUP:
        case WM_KEYUP: {
            core.Input.keysNew[(int)wParam] = false;
            core.Input.keysRepeat[(int)wParam] = 0;
            return 0;
        }

        case WM_SYSCHAR:
        case WM_CHAR: {
            core.Input.lastChar = char(int(wParam));
            return 0;
        }

        // extra
        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT) {
                SetCursor(core.Window.cursor);
                return TRUE;
            }
        }
        case WM_SETTINGCHANGE: {
            gi_UpdateSettings();
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Timer (all of it is taken from raylib)
LARGE_INTEGER GetTimerFrequency() {
    LARGE_INTEGER frequency = {{0, 0}};
	if (frequency.QuadPart == 0) {
		timeBeginPeriod(1);
		QueryPerformanceFrequency(&frequency);
	}
    return frequency;
}

void gsgl_InitTimer() {
    Logger_log(LOGGER_INFO, "GRAPHICS: Initiating timer");

    // Setting a higher resolution can improve the accuracy of time-out intervals in wait functions
    // However, it can also reduce overall system performance, because the thread scheduler switches tasks more often
    // High resolutions can also prevent the CPU power management system from entering power-saving modes
    // Setting a higher resolution does not improve the accuracy of the high-resolution performance counter
    #if defined(_WIN32) && defined(SUPPORT_WINMM_HIGHRES_TIMER) && !defined(SUPPORT_BUSY_WAIT_LOOP)
    GetTimerFrequency(); // this literally just initializes it lol
    #endif

    #if defined(__linux__)
    struct timespec now = { 0 };

    if (clock_gettime(CLOCK_MONOTONIC, &now) == 0) { // Success
        core.Time.base = (unsigned long long int)now.tv_sec*1000000000LLU + (unsigned long long int)now.tv_nsec;
    }
    #endif

    core.Time.previous = gsgl_GetTime();     // Get time as double
}

double gsgl_GetTime() {
    /*double time = glfwGetTime();   // Elapsed time since glfwInit()
    return time;*/
    LARGE_INTEGER frequency = GetTimerFrequency();
    LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return (double) (counter.QuadPart / (double) frequency.QuadPart);
}

void gsgl_WaitTime(double seconds) {
    if (seconds < 0) return;    // Security check

#if defined(SUPPORT_BUSY_WAIT_LOOP) || defined(SUPPORT_PARTIALBUSY_WAIT_LOOP)
    double destinationTime = gsgl_GetTime() + seconds;
#endif

#if defined(SUPPORT_BUSY_WAIT_LOOP)
    while (gsgl_GetTime() < destinationTime) { }
#else
    #if defined(SUPPORT_PARTIALBUSY_WAIT_LOOP)
        double sleepSeconds = seconds - seconds*0.05;  // NOTE: We reserve a percentage of the time for busy waiting
    #else
        double sleepSeconds = seconds;
    #endif

    // System halt functions
    #if defined(_WIN32)
        Sleep((unsigned long)(sleepSeconds*1000.0));
    #endif
    #if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__EMSCRIPTEN__)
        struct timespec req = { 0 };
        time_t sec = sleepSeconds;
        long nsec = (sleepSeconds - sec)*1000000000L;
        req.tv_sec = sec;
        req.tv_nsec = nsec;

        // NOTE: Use nanosleep() on Unix platforms... usleep() it's deprecated
        while (nanosleep(&req, &req) == -1) continue;
    #endif
    #if defined(__APPLE__)
        usleep(sleepSeconds*1000000.0);
    #endif

    #if defined(SUPPORT_PARTIALBUSY_WAIT_LOOP)
        while (gsgl_GetTime() < destinationTime) { }
    #endif
#endif
}

float gsgl_GetFrameTime() {
    return (float)core.Time.frame;
}

// input
// mouse
Vector2i gsgl_GetMousePosition() {
    return {core.Input.mouseNew.mouseX, core.Input.mouseNew.mouseY};
}

bool gsgl_IsMouseButtonDown(GSGL_MouseButton button) {
    if (button == GSGL_LMB) return core.Input.mouseNew.leftMouseButton == true;
    else if (button == GSGL_RMB) return core.Input.mouseNew.rightMouseButton == true;
    else if (button == GSGL_MMB) return core.Input.mouseNew.middleMouseButton == true;
    return false;
}
bool gsgl_IsMouseButtonUp(GSGL_MouseButton button) {
    return !gsgl_IsMouseButtonDown(button);
}

bool gsgl_IsMouseButtonPressed(GSGL_MouseButton button) {
    if (button == GSGL_LMB) if (core.Input.mouseNew.leftMouseButton == true && core.Input.mouseOld.leftMouseButton == false) return true;
    else if (button == GSGL_RMB) if (core.Input.mouseNew.rightMouseButton == true && core.Input.mouseOld.rightMouseButton == false) return true;
    else if (button == GSGL_MMB) if (core.Input.mouseNew.middleMouseButton == true && core.Input.mouseOld.middleMouseButton == false) return true;
    return false;
}
bool gsgl_IsMouseButtonReleased(GSGL_MouseButton button) {
    if (button == GSGL_LMB) if (core.Input.mouseNew.leftMouseButton == false && core.Input.mouseOld.leftMouseButton == true) return true;
    else if (button == GSGL_RMB) if (core.Input.mouseNew.rightMouseButton == false && core.Input.mouseOld.rightMouseButton == true) return true;
    else if (button == GSGL_MMB) if (core.Input.mouseNew.middleMouseButton == false && core.Input.mouseOld.middleMouseButton == true) return true;
    return false;
}

void gsgl_SetCursor(GSGL_Cursor cursor) {
    if (gsgl_IsWindowMinimized()) return;

    core.Input.cursorStyle = cursor;
    core.Input.cursorChanged = true;
}

// key
bool gsgl_IsKeyDown(GSGL_Key key) {
    if (core.Input.keysNew[key] == true) return true;
    else return false;
}
bool gsgl_IsKeyUp(GSGL_Key key) {
    return !gsgl_IsKeyDown(key);
}

bool gsgl_IsKeyPressed(GSGL_Key key) {
    if (core.Input.keysNew[key] == true && core.Input.keysOld[key] == false) return true;
    else return false;
}
bool gsgl_IsKeyReleased(GSGL_Key key) {
    if (core.Input.keysNew[key] == false && core.Input.keysOld[key] == true) return true;
    else return false;
}

bool gsgl_IsKeyRepeat(GSGL_Key key) {
    // this may or may not be the best idea.
    // handle the repeating logic here

    if (core.Input.keysNew[key] == true && core.Input.keysOld[key] == false) {
        core.Input.keysRepeat[key] = core.Input.repeatDelay;
        return true;
    } else if (core.Input.keysNew[key] == false && core.Input.keysOld[key] == true) {
        core.Input.keysRepeat[key] = 0;
        return false;
    }

    if (core.Input.keysNew[key] == false) {
        core.Input.keysRepeat[key] = 0;
        return false;
    } else {
        core.Input.keysRepeat[key] -= int(gsgl_GetFrameTime()*60);
        if (core.Input.keysRepeat[key] <= 0) {
            core.Input.keysRepeat[key] = core.Input.repeatSpeed;
            return true;
        }
        return false;
    }

    // this should never be actually hit
    return false;
}

char gsgl_GetLastChar() {
    return core.Input.lastChar;
}

// clipboard
const char* gsgl_GetClipboardText() {
    if (!IsClipboardFormatAvailable(CF_TEXT)) {
        Logger_log(LOGGER_ERROR, "CLIPBOARD: Text format not available (Get)");
        return "";
    }
    if (!OpenClipboard(core.Window.handle)) {
        Logger_log(LOGGER_ERROR, "CLIPBOARD: Couldn't open clipboard (Get)");
        gsgl_GetLastError();
        return "";
    }

    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == nullptr) {
        Logger_log(LOGGER_ERROR, "CLIPBOARD: Received data handle was NULL");
        CloseClipboard();
        return "";
    }

    WCHAR* buffer = (WCHAR*)GlobalLock(hData);
    if (!buffer) {
        Logger_log(LOGGER_ERROR, "CLIPBOARD: Received clipboard data was NULL");
        CloseClipboard();
        gsgl_GetLastError();
        return "";
    }

    const char* pszText = (const char*)buffer;

    GlobalUnlock(hData);

    CloseClipboard();

    return pszText;
}
GSGL_API void gsgl_SetClipboardText(const char* txt) {
    if (!OpenClipboard(core.Window.handle)) {
        Logger_log(LOGGER_ERROR, "CLIPBOARD: Couldn't open clipboard (Set)");
        gsgl_GetLastError();
        return;
    }

    EmptyClipboard();

    // we have to do soome SILLYYY stuff to get it into the clipboard.
    const size_t len = strlen(txt) + 1;
    HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, len);
    memcpy(GlobalLock(hMem), txt, len);
    GlobalUnlock(hMem);
    if (!SetClipboardData(CF_TEXT, hMem)) {
        Logger_log(LOGGER_ERROR, "CLIPBOARD: Couldn't set clipboard (Set)");
        gsgl_GetLastError();
    }
    CloseClipboard();
}