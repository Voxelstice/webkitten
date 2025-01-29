// NOTE: curl is including windows.h
// maybe make a file that just wraps some raylib functions into their own functions

// TODO: add https://github.com/Sigil-Ebook/cssparser/tree/master/cssparser

/*

    the design here is as follows:
    - We have a Handler class that handles everything
    - We have a Renderer class that handles the window
    - We have a Networker class that handles networking
    - We have a Scripter class that handles code

*/

#include "classes/main/handler.h"
#include "classes/main/renderer.h"
#include "classes/main/networkerBarrier.h"
#include "classes/main/scripter.h"

#include "logger.h"

Handler* handler;
Renderer* renderer;
Scripter* scripter;

int main() {
    // Initialization
    Logger_init();

    Logger_log(LOGGER_INFO, "Application started");
    Logger_log(LOGGER_INFO, "tinyweb - a lightweight browser by Voxelstice");
    #ifdef BITNESS64
    Logger_log(LOGGER_INFO, "Running the 64-bit build");
    #endif
    #ifdef BITNESS32
    Logger_log(LOGGER_INFO, "Running the 32-bit build");
    #endif
    Logger_log(LOGGER_INFO, "----------------------------------------------------------------------------------");

    // initialize handlers
    handler = new Handler();
    renderer = new Renderer();
    scripter = new Scripter();

    renderer->init();
    handler->init();
    networker_init();
    scripter->init();

    //----------------------------------------------------------------------------------

    while (!renderer->shouldClose()) {
        // Update
        handler->update();
        renderer->update();
        scripter->update();

        //----------------------------------------------------------------------------------

        // Draw
        renderer->draw();
        //----------------------------------------------------------------------------------
    }

    Logger_log(LOGGER_INFO, "----------------------------------------------------------------------------------");
    Logger_log(LOGGER_INFO, "Application closing");

    // De-Initialization
    //--------------------------------------------------------------------------------------
    renderer->close();
    //--------------------------------------------------------------------------------------

    Logger_log(LOGGER_INFO, "----------------------------------------------------------------------------------");
    Logger_close();

    return 0;
}