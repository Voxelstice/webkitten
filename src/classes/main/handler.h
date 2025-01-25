#pragma once

#include <vector>
#include "../tab/tab.h"

class Handler {
    public:
        Handler();

        void init();
        void update();
        void draw();

        void focusTab(int id);
        void closeTab(int id);

        std::vector<Tab*> tabs;
        int tabFocus = 0;
        bool ready = false;
    private:
        
};