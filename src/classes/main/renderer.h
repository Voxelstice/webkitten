#pragma once

#include "../../internal/gsgl/gsgl.h"

class Renderer {
    public:
        Renderer();

        void init();
        void update();
        void draw();

        void close();
        bool shouldClose();

    private:
        bool closing = false;
};