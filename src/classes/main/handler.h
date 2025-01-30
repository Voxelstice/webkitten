#pragma once

#include <vector>
#include "../tab/tab.h"
#include "../ui/classes/input.h"

class Handler {
    public:
        Handler();

        void init();
        void update();
        void draw();

        void focusTab(int id);
        void closeTab(int id);
        int getTab(int id);

        void drawInput(Vector2i pos, Vector2i size);

        std::vector<Tab*> tabs;
        int tabFocus = 0;
        bool ready = false;
    private:
        Input* input;
};