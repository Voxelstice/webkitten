#pragma once

#include <curl/curl.h>
#include "../../logger.h"

class Networker {
    public:
        Networker();

        void init();
        void update();
        void draw();

        void CheckCode(CURLcode code);
};