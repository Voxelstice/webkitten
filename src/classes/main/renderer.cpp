#include "../../main.h"
#include "../ui/fonts.h"
#include "../ui/ui.h"
#include "../tab/tab.h"
#include "../../logger.h"
#include "renderer.h"

#include <string>

static Input* input;

Renderer::Renderer() {
    // do nothing
}

void Renderer::init() {
    // initialize window
    gsgl_SoftwareRender();
    gsgl_InitWindow(1600, 900, "tinyweb");
    gsgl_SetFrameRate(60);

    Logger_log(LOGGER_INFO, "----------------------------------------------------------------------------------");

    LoadFonts();
    input = new Input(GetFont(PROGGY_CLEAN), 24, "Enter web address", {160, 160, 160, 255}, {255, 255, 255, 255});
}
void Renderer::update() {
    input->update();
}
void Renderer::draw() {
    gsgl_PollEvents();

    // draw the top bar
    gsgl_Rect(0, 0, gsgl_GetScreenWidth(), 32, {48, 48, 48, 255});

    // draw the tabs
    int offset = 0;
    if (handler != nullptr) {
        if (handler->ready == true) {
            int tabWidth = gsgl_GetScreenWidth() / 8;
            int textMaxLength = (tabWidth / 8)-2;

            for (Tab* tab : handler->tabs) {
                bool tabHovered = gsgl_IsPointInRect(gsgl_GetMousePosition(), {0 + offset, 0}, {tabWidth, 32});
                //bool closeHovered = gsgl_IsPointInRect(gsgl_GetMousePosition(), {tabWidth - 28 + offset, 4}, {24, 24});

                bool closeBtn = false;

                if (handler->tabFocus == tab->getId()) {
                    gsgl_Rect(0 + offset, 0, tabWidth, 32, {128, 128, 128, 255});
                    gsgl_Rect(4 + offset, 4, tabWidth-8, 24, {64, 64, 64, 255});
                    
                    closeBtn = ui_BasicButton(GetFont(PROGGY_CLEAN), "X", 16, {tabWidth - 28 + offset, 4}, {24, 24}, {255, 255, 255, 255}, {128, 128, 128, 255}, {160, 160, 160, 255}, {255, 255, 255, 255});
                } else {
                    if (tabHovered == true) {
                        gsgl_Rect(0 + offset, 0, tabWidth, 32, {128, 128, 128, 255});
                        gsgl_Rect(4 + offset, 4, tabWidth-8, 24, {96, 96, 96, 255});


                        closeBtn = ui_BasicButton(GetFont(PROGGY_CLEAN), "X", 16, {tabWidth - 28 + offset, 4}, {24, 24}, {255, 255, 255, 255}, {128, 128, 128, 255}, {160, 160, 160, 255}, {255, 255, 255, 255});
                    } else {
                        gsgl_Rect(0 + offset, 0, tabWidth, 32, {64, 64, 64, 255});
                        gsgl_Rect(4 + offset, 4, tabWidth-8, 24, {48, 48, 48, 255});

                        gsgl_Rect(tabWidth - 28 + offset, 4, 24, 24, {64, 64, 64, 255});
                        closeBtn = ui_BasicButton(GetFont(PROGGY_CLEAN), "X", 16, {tabWidth - 28 + offset, 4}, {24, 24}, {255, 255, 255, 255}, {64, 64, 64, 255}, {64, 64, 64, 255}, {64, 64, 64, 255});
                    }
                }

                if (tabHovered == true && gsgl_IsMouseButtonPressed(GSGL_LMB)) handler->focusTab(tab->getId());
                if (closeBtn == true) handler->closeTab(tab->getId());

                gsgl_ScissorsStart(8 + offset, 4, tabWidth - 40, 24);
                gsgl_DrawText(GetFont(PROGGY_CLEAN), tab->getTitle().c_str(), 8 + offset, 10, 16, {255, 255, 255, 255});
                gsgl_ScissorsStop();

                offset += tabWidth;
            }
        } else if (handler->ready == false) {
            gsgl_DrawText(GetFont(PROGGY_CLEAN), "The handler is still initializing", 4, 4, 24, {255, 255, 255, 255});
        }
    } else {
        gsgl_DrawText(GetFont(PROGGY_CLEAN), "The handler is null, this shouldn't happen!", 4, 4, 24, {255, 255, 255, 255});
    }

    // draw the address bar
    gsgl_Rect(0, 32, gsgl_GetScreenWidth(), 32, {64, 64, 64, 255});

    // draw search bar
    Vector2i searchPos = {int(gsgl_GetScreenWidth() * 0.15), 32 + 4};
    Vector2i searchSize = {int(gsgl_GetScreenWidth() * 0.7), 24};

    gsgl_Rect(searchPos.x, searchPos.y, searchSize.x, searchSize.y, {96, 96, 96, 255});
    input->setRect(searchPos, searchSize);
    input->draw();

    // draw buttons
    bool historyBackward = ui_BasicButton(GetFont(PROGGY_CLEAN), "B", 16, {4, 32 + 4}, {24, 24}, {255, 255, 255, 255}, {64, 64, 64, 255}, {128, 128, 128, 255}, {160, 160, 160, 255});
    bool historyForward = ui_BasicButton(GetFont(PROGGY_CLEAN), "F", 16, {4 + 32, 32 + 4}, {24, 24}, {255, 255, 255, 255}, {64, 64, 64, 255}, {128, 128, 128, 255}, {160, 160, 160, 255});
    bool reloadPage = ui_BasicButton(GetFont(PROGGY_CLEAN), "R", 16, {4 + 64, 32 + 4}, {24, 24}, {255, 255, 255, 255}, {64, 64, 64, 255}, {128, 128, 128, 255}, {160, 160, 160, 255});

    gsgl_Clear({0, 0, 0, 255});
    gsgl_SwapBuffers();
    gsgl_Draw();
}

void Renderer::close() {
    closing = true;
    gsgl_CloseWindow();
}
bool Renderer::shouldClose() {
    return closing || gsgl_ShouldClose();
}