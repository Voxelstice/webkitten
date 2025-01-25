#pragma once

#include <curl/curl.h>
#include "../../logger.h"

class Request {
    public:
        Request();

        void init();

        CURL* curl;
};