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

- X11 support is a bit weird.

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
#include <cmath>
#include <iostream>

#ifdef _WIN32
#include <comdef.h>
#include <windows.h>
#include <windowsx.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h> // for some weird reason you need to include this for memset
#endif

#include "../../logger.h"
#include "gsgl.h"

#define SUPPORT_WINMM_HIGHRES_TIMER      1
#define SUPPORT_PARTIALBUSY_WAIT_LOOP    1
#define KEYBOARD_KEYS                    512

#ifdef _WIN32
typedef struct Win32Core {
    HINSTANCE instance;
    struct {
        HWND handle;
        MSG msg;
        HCURSOR cursor;
    } Window;
} Win32Core;
Win32Core platformCore = { 0 };
#else
typedef struct LinuxCore {
    Display* display;
    Window window;
    unsigned int eventMask;

    bool destroyed;

    struct {
        XImage* image;

        int depth;
        XVisualInfo vinfo;
        XGCValues gcv;
        unsigned long gcm;
        GC context;
    } Graphics;
} LinuxCore;
LinuxCore platformCore = { 0 };
#endif

typedef struct GraphicsCore {
    bool ready;
    struct {
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
#ifndef _WIN32
GSGL_Key gi_XSymToGSGLKey(KeySym sym);
#endif
void gi_UpdateSettings();
void gi_ResizeWindow(int width, int height);
void gi_InitBuffers();

#ifdef _WIN32
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
        MessageBox(NULL, "FormatMessage failed", "webkitten", MB_OK);
        gsgl_CloseWindow();
    }

    Logger_log(LOGGER_ERROR, "GRAPHICS: Internal error: %s", lpMsgBuf);
    MessageBox(NULL, (LPCSTR)lpMsgBuf, "webkitten", MB_OK);

    LocalFree(lpMsgBuf);
    gsgl_CloseWindow();
    
    //ExitProcess(dw); 
}
#else
void gsgl_GetLastError() {
    // TODO: implement this
}
#endif

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
    #ifdef _WIN32
    platformCore.instance = GetModuleHandle(0);
    platformCore.Window.msg = { };
    #else
    platformCore.destroyed = false;
    #endif

    core.ready = false;

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

    #ifdef _WIN32
    // set class name
    const char* className = "webkitten";

    // register class
    WNDCLASS winClass = { };

    winClass.lpfnWndProc = WindowProc;
    winClass.hInstance = platformCore.instance;
    winClass.lpszClassName = className;

    Logger_log(LOGGER_INFO, "GRAPHICS: - Registering class");
    RegisterClass(&winClass);

    // create window
    Logger_log(LOGGER_INFO, "GRAPHICS: - Creating window");
    platformCore.Window.handle = CreateWindowEx(
        0, className, title, WS_OVERLAPPEDWINDOW,
        0, 0, width, height,
        NULL, NULL, platformCore.instance, NULL
    );

    if (platformCore.Window.handle == NULL) {
        gsgl_GetLastError();
        return;
    } else {
        Logger_log(LOGGER_INFO, "GRAPHICS: - Window successfully created");
    }

    // show it
    Logger_log(LOGGER_INFO, "GRAPHICS: - Showing window");
    ShowWindow(platformCore.Window.handle, SW_NORMAL);

    // set cursor
    platformCore.Window.cursor = LoadCursor(NULL, IDC_ARROW);
    SetCursor(platformCore.Window.cursor);
    #else
    // open display
    Logger_log(LOGGER_INFO, "GRAPHICS: - Opening display");
    platformCore.display = XOpenDisplay(NULL);
    if (platformCore.display == NULL) {
        gsgl_GetLastError();
        return;
    } else {
        Logger_log(LOGGER_INFO, "GRAPHICS: - Display successfully opened");
    }

    // we need a bit more setup for graphics aswell
    // https://stackoverflow.com/a/64758878
    XVisualInfo *visual_list;
    XVisualInfo visual_template;
    Visual *visual;

    int nxvisuals;

    Logger_log(LOGGER_INFO, "GRAPHICS: - Preparing window graphics");
    visual_template.screen = DefaultScreen(platformCore.display);
    visual_list = XGetVisualInfo(platformCore.display, VisualScreenMask, &visual_template, &nxvisuals);

    if (!XMatchVisualInfo(platformCore.display, XDefaultScreen(platformCore.display), 24, TrueColor, &platformCore.Graphics.vinfo)) {
        Logger_log(LOGGER_ERROR, "GRAPHICS: No such visual graphics");
        return;
    }

    XSync(platformCore.display, true);

    visual = platformCore.Graphics.vinfo.visual;
    platformCore.Graphics.depth = platformCore.Graphics.vinfo.depth;

    XSetWindowAttributes attrs;
    attrs.colormap = XCreateColormap(platformCore.display, XDefaultRootWindow(platformCore.display), visual, AllocNone);
    attrs.background_pixel = 0;
    attrs.border_pixel = 0;

    // create window
    Logger_log(LOGGER_INFO, "GRAPHICS: - Creating window");
    platformCore.window = XCreateWindow(
        platformCore.display, DefaultRootWindow(platformCore.display),
        50, 50, width, height, 
        0, platformCore.Graphics.depth, InputOutput, visual, CWBackPixel | CWColormap | CWBorderPixel, &attrs
    );
    XStoreName(platformCore.display, platformCore.window, title);
    
    if (platformCore.window == NULL) {
        gsgl_GetLastError();
        return;
    } else {
        Logger_log(LOGGER_INFO, "GRAPHICS: - Window successfully created");
    }

    platformCore.eventMask = StructureNotifyMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
    XSelectInput(platformCore.display, platformCore.window, platformCore.eventMask);

    // final graphics steps
    platformCore.Graphics.gcm = GCGraphicsExposures;
    platformCore.Graphics.gcv.graphics_exposures = 0;
    platformCore.Graphics.context = XCreateGC(
        platformCore.display, DefaultRootWindow(platformCore.display), 
        platformCore.Graphics.gcm, &platformCore.Graphics.gcv
    );

    // show
    Logger_log(LOGGER_INFO, "GRAPHICS: - Showing window");
    XMapWindow(platformCore.display, platformCore.window);
	
    // need to do this to listen to delete window events
    Atom wmDelete = XInternAtom(platformCore.display, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(platformCore.display, platformCore.window, &wmDelete, 1);

    #endif

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
    
    #ifdef _WIN32
    int res = GetMessage(&platformCore.Window.msg, platformCore.Window.handle, 0, 0);
    if (res == 0) {
        gsgl_CloseWindow();
    } else {
        TranslateMessage(&platformCore.Window.msg);
        DispatchMessage(&platformCore.Window.msg);
    }
    #else

    if (XPending(platformCore.display)) {
        XEvent event;
        XNextEvent(platformCore.display, &event);
        switch (event.type) {
            // Resizing
            case ConfigureNotify: {
                XConfigureEvent* config = (XConfigureEvent*)&event;
                gi_ResizeWindow(config->width, config->height);
                break;
            }

            // Input
            // since X11 handles keycodes in a different breed we need to translate the layouts to our own layout
            case KeyPress: {
                XKeyPressedEvent* key = (XKeyPressedEvent*)&event;
                GSGL_Key gsglKey = gi_XSymToGSGLKey(XKeycodeToKeysym(platformCore.display, key->keycode, 0));
                core.Input.keysNew[(int)gsglKey] = true;
                break;
            }
            case KeyRelease: {
                XKeyReleasedEvent* key = (XKeyReleasedEvent*)&event;
                GSGL_Key gsglKey = gi_XSymToGSGLKey(XKeycodeToKeysym(platformCore.display, key->keycode, 0));
                core.Input.keysNew[(int)gsglKey] = false;
                break;
            }

            // mouse buttons
            case ButtonPress: {
                XButtonPressedEvent* btn = (XButtonPressedEvent*)&event;
                if (btn->button == Button1) core.Input.mouseNew.leftMouseButton = true;
                if (btn->button == Button2) core.Input.mouseNew.middleMouseButton = true;
                if (btn->button == Button3) core.Input.mouseNew.rightMouseButton = true;
                break;
            }
            case ButtonRelease: {
                XButtonReleasedEvent* btn = (XButtonReleasedEvent*)&event;
                if (btn->button == Button1) core.Input.mouseNew.leftMouseButton = false;
                if (btn->button == Button2) core.Input.mouseNew.middleMouseButton = false;
                if (btn->button == Button3) core.Input.mouseNew.rightMouseButton = false;
                break;
            }

            // causes performance problems
            /*case MotionNotify: {
                XPointerMovedEvent* pointer = (XPointerMovedEvent*)&event;
                core.Input.mouseNew.mouseX = pointer->x;
                core.Input.mouseNew.mouseY = pointer->y;
                break;
            }*/

            // Used for handling closing properly
            case ClientMessage: {
                XClientMessageEvent *Event = (XClientMessageEvent *) &event;
                Atom wmDelete = XInternAtom(platformCore.display, "WM_DELETE_WINDOW", True);
                if((Atom)Event->data.l[0] == wmDelete) {
                    platformCore.destroyed = true;
                    gsgl_CloseWindow();
                }
                break;
            }

            default: {
                break;
            }
        }
    }
    #endif
}

bool gsgl_ShouldClose() {
    return core.Window.closing;
}

void gsgl_CloseWindow() {
    core.Window.closing = true;

    #ifdef _WIN32
    PostQuitMessage(0);
    #else
    if (platformCore.destroyed == false) {
        platformCore.destroyed = true;
        if (platformCore.window != NULL && platformCore.display != NULL) XDestroyWindow(platformCore.display, platformCore.window);
        if (platformCore.display != NULL) XCloseDisplay(platformCore.display);
    }
    #endif
}

int gsgl_GetScreenWidth() {
    return core.Window.width;
}
int gsgl_GetScreenHeight() {
    return core.Window.height;
}

bool gsgl_IsWindowMinimized() {
    #ifdef _WIN32
    return IsIconic(platformCore.Window.handle);
    #else
    return false;
    #endif
}
bool gsgl_IsWindowMaximized() {
    #ifdef _WIN32
    return IsZoomed(platformCore.Window.handle);
    #else
    return false;
    #endif
}
bool gsgl_IsWindowVisible() {
    // Currently functions like IsWindowMinimized.
    // This is just future proofing when I need it at one point.
    #ifdef _WIN32
    return IsIconic(platformCore.Window.handle);
    #else
    return false;
    #endif
}

// Graphics functions
void gsgl_Draw() {
    if (gsgl_IsWindowVisible()) return;

    #ifdef _WIN32
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = core.Window.width;
    bmi.bmiHeader.biHeight = -core.Window.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(platformCore.Window.handle);
    StretchDIBits(hdc, 0, 0, core.Window.width, core.Window.height, 0, 0, core.Window.width, core.Window.height, core.Graphics.buffer2, &bmi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(platformCore.Window.handle, hdc);
    #else
    if (
        platformCore.Graphics.image != NULL && platformCore.Graphics.context && 
        platformCore.Graphics.image->width == core.Window.width && platformCore.Graphics.image->height == core.Window.height
    ) {
        XPutImage(
            platformCore.display, platformCore.window, 
            platformCore.Graphics.context, platformCore.Graphics.image, 
            0, 0, 0, 0, core.Window.width, core.Window.height
        );
    }
    #endif
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
        #ifdef _WIN32
        if (core.Input.cursorStyle == GSGL_POINTER) {
            platformCore.Window.cursor = LoadCursor(NULL, IDC_ARROW);
        } else if (core.Input.cursorStyle == GSGL_CLICK) {
            platformCore.Window.cursor = LoadCursor(NULL, IDC_HAND);
        } else if (core.Input.cursorStyle == GSGL_TEXT) {
            platformCore.Window.cursor = LoadCursor(NULL, IDC_IBEAM);
        }
        SetCursor(platformCore.Window.cursor);
        #else

        #endif

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

    int x = (int)std::floor(index % core.Window.width);
    int y = (int)std::floor(index / core.Window.width);
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
#ifndef _WIN32

// WARNING: Right variants of some keys will return left variant (Shift, Control, Alt, Super)
GSGL_Key gi_XSymToGSGLKey(KeySym sym) {
    switch (sym) {
        case XK_BackSpace: return KEY_BACKSPACE;
        case XK_Tab: return KEY_TAB;
        case XK_ISO_Enter: return KEY_ENTER;
        case XK_Shift_L: return KEY_LEFT_SHIFT;
        case XK_Shift_R: return KEY_LEFT_SHIFT;
        case XK_Control_L: return KEY_LEFT_CONTROL;
        case XK_Control_R: return KEY_LEFT_CONTROL;
        case XK_Alt_L: return KEY_LEFT_ALT;
        case XK_Alt_R: return KEY_LEFT_ALT;
        case XK_Pause: return KEY_PAUSE;
        case XK_Caps_Lock: return KEY_CAPS_LOCK;
        case XK_Escape: return KEY_ESCAPE;
        case XK_space: return KEY_SPACE;
        case XK_Page_Up: return KEY_PAGE_UP;
        case XK_Page_Down: return KEY_PAGE_DOWN;
        case XK_End: return KEY_END;
        case XK_Home: return KEY_HOME;
        case XK_Left: return KEY_LEFT;
        case XK_Up: return KEY_UP;
        case XK_Right: return KEY_RIGHT;
        case XK_Down: return KEY_DOWN;
        case XK_Insert: return KEY_INSERT;
        case XK_Delete: return KEY_DELETE;

        case XK_0: return KEY_ZERO;
        case XK_1: return KEY_ONE;
        case XK_2: return KEY_TWO;
        case XK_3: return KEY_THREE;
        case XK_4: return KEY_FOUR;
        case XK_5: return KEY_FIVE;
        case XK_6: return KEY_SIX;
        case XK_7: return KEY_SEVEN;
        case XK_8: return KEY_EIGHT;
        case XK_9: return KEY_NINE;

        case XK_A: return KEY_A;
        case XK_B: return KEY_B;
        case XK_C: return KEY_C;
        case XK_D: return KEY_D;
        case XK_E: return KEY_E;
        case XK_F: return KEY_F;
        case XK_G: return KEY_G;
        case XK_H: return KEY_H;
        case XK_I: return KEY_I;
        case XK_J: return KEY_J;
        case XK_K: return KEY_K;
        case XK_L: return KEY_L;
        case XK_M: return KEY_M;
        case XK_N: return KEY_N;
        case XK_O: return KEY_O;
        case XK_P: return KEY_P;
        case XK_Q: return KEY_Q;
        case XK_R: return KEY_R;
        case XK_S: return KEY_S;
        case XK_T: return KEY_T;
        case XK_U: return KEY_U;
        case XK_V: return KEY_V;
        case XK_W: return KEY_W;
        case XK_X: return KEY_X;
        case XK_Y: return KEY_Y;
        case XK_Z: return KEY_Z;

        case XK_Super_L: return KEY_LEFT_SUPER;
        case XK_Super_R: return KEY_LEFT_SUPER;

        case XK_F1: return KEY_F1;
        case XK_F2: return KEY_F2;
        case XK_F3: return KEY_F3;
        case XK_F4: return KEY_F4;
        case XK_F5: return KEY_F5;
        case XK_F6: return KEY_F6;
        case XK_F7: return KEY_F7;
        case XK_F8: return KEY_F8;
        case XK_F9: return KEY_F9;
        case XK_F10: return KEY_F10;
        case XK_F11: return KEY_F11;
        case XK_F12: return KEY_F12;
        case XK_Scroll_Lock: return KEY_SCROLL_LOCK;

        case XK_semicolon: return KEY_SEMICOLON;
        case XK_equal: return KEY_EQUAL;
        case XK_comma: return KEY_COMMA;
        case XK_minus: return KEY_MINUS;
        case XK_period: return KEY_PERIOD;
        case XK_slash: return KEY_SLASH;
        case XK_grave: return KEY_GRAVE;
        case XK_bracketleft: return KEY_LEFT_BRACKET;
        case XK_backslash: return KEY_BACKSLASH;
        case XK_bracketright: return KEY_RIGHT_BRACKET;
        case XK_apostrophe: return KEY_APOSTROPHE;
        default: break;
    }
    return KEY_NULL;
}
#endif
void gi_UpdateSettings() {
    // update some stuff. not a lot needs to be updated here currently
    // this seems to fire somewhat frequently but its kind of weird when it updates

    #ifdef _WIN32
    DWORD repeatDelay;
    DWORD repeatSpeed;

    SystemParametersInfoA(SPI_GETKEYBOARDDELAY, 0, &repeatDelay, 0);
    SystemParametersInfoA(SPI_GETKEYBOARDSPEED, 0, &repeatSpeed, 0);

    core.Input.repeatDelay = int(float(repeatDelay) * 60);
    core.Input.repeatSpeed = int(roundf((1 / float(repeatSpeed+1)) * 60));
    #else

    #endif
}
void gi_ResizeWindow(int width, int height) {
    if (core.Window.width != width || core.Window.height != height) { // check if it's the same, otherwise do not reinitialize buffers
        core.Window.width = width;
        core.Window.height = height;

        gi_InitBuffers();
    }
}
void gi_InitBuffers() {
    if (core.Graphics.software == true) {
        if (core.Graphics.buffersInited == true) free(core.Graphics.buffer1);
        core.Graphics.buffer1 = (uint32_t*) malloc(core.Window.width * core.Window.height * sizeof(uint32_t));
        memset(core.Graphics.buffer1, 0, core.Window.width * core.Window.height * sizeof(uint32_t));

        if (core.Graphics.buffersInited == true) free(core.Graphics.buffer2);
        core.Graphics.buffer2 = (uint32_t*) malloc(core.Window.width * core.Window.height * sizeof(uint32_t));
        memset(core.Graphics.buffer2, 0, core.Window.width * core.Window.height * sizeof(uint32_t));

        #ifndef _WIN32
        platformCore.Graphics.image = XCreateImage(
            platformCore.display, platformCore.Graphics.vinfo.visual, platformCore.Graphics.depth, ZPixmap, 
            0, (char*)core.Graphics.buffer2, core.Window.width, core.Window.height, 8, core.Window.width*4
        );
        #endif

        if (core.Graphics.buffersInited == false)
            core.Graphics.buffersInited = true;
    }
}

#ifdef _WIN32
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
                SetCursor(platformCore.Window.cursor);
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
#else

#endif

// Timer (all of it is taken from raylib)
#ifdef _WIN32
LARGE_INTEGER GetTimerFrequency() {
    LARGE_INTEGER frequency = {{0, 0}};
	if (frequency.QuadPart == 0) {
		timeBeginPeriod(1);
		QueryPerformanceFrequency(&frequency);
	}
    return frequency;
}
#endif

void gsgl_InitTimer() {
    Logger_log(LOGGER_INFO, "GRAPHICS: Initiating timer");

    #if defined(_WIN32) && defined(SUPPORT_WINMM_HIGHRES_TIMER) && !defined(SUPPORT_BUSY_WAIT_LOOP)
    GetTimerFrequency();
    #endif

    #if defined(__linux__)
    struct timespec now = { 0 };

    if (clock_gettime(CLOCK_MONOTONIC, &now) == 0) {
        core.Time.base = (unsigned long long int)now.tv_sec*1000000000LLU + (unsigned long long int)now.tv_nsec;
    }
    #endif

    core.Time.previous = gsgl_GetTime();
}

double gsgl_GetTime() {
    #ifdef _WIN32
    LARGE_INTEGER frequency = GetTimerFrequency();
    LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
    return (double) (counter.QuadPart / (double) frequency.QuadPart);
    #else
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) == 0) { // Success
        return (double)now.tv_sec*1000000000LLU + (double)now.tv_nsec;
    }
    #endif
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
    #ifndef _WIN32
    Window child_win, root_win;
    int rootX, rootY;
    unsigned int mask;
    XQueryPointer(platformCore.display, platformCore.window, &child_win, &root_win, &rootX, &rootY, &core.Input.mouseNew.mouseX, &core.Input.mouseNew.mouseY, &mask);
    #endif

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
    #ifdef _WIN32
    if (!IsClipboardFormatAvailable(CF_TEXT)) {
        Logger_log(LOGGER_ERROR, "CLIPBOARD: Text format not available (Get)");
        return "";
    }
    if (!OpenClipboard(platformCore.Window.handle)) {
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
    #else
    return "";
    #endif
}
GSGL_API void gsgl_SetClipboardText(const char* txt) {
    #ifdef _WIN32
    if (!OpenClipboard(platformCore.Window.handle)) {
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
    #else

    #endif
}