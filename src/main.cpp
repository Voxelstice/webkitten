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
#include "classes/main/networker.h"
#include "classes/main/scripter.h"

#include "logger.h"

Handler* handler;
Renderer* renderer;
Networker* networker;
Scripter* scripter;

int main() {
    // Initialization
    Logger_init();

    Logger_log(LOGGER_INFO, "Application started");
    Logger_log(LOGGER_INFO, "webkitten - a lightweight browser by Voxelstice");

    #ifdef BITNESS64
    Logger_log(LOGGER_INFO, "Running the 64-bit build");
    #endif
    #ifdef BITNESS32
    Logger_log(LOGGER_INFO, "Running the 32-bit build");
    #endif

    Logger_log(LOGGER_INFO, "----------------------------------------------------------------------------------");

    // initialize handlers
    networker = new Networker();
    handler = new Handler();
    renderer = new Renderer();
    scripter = new Scripter();

    networker->init();
    renderer->init();
    handler->init();
    scripter->init();

    Logger_log(LOGGER_INFO, "----------------------------------------------------------------------------------");

    //----------------------------------------------------------------------------------

    while (!renderer->shouldClose()) {
        // Update
        networker->update();
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