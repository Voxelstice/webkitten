#pragma once

#include <functional>
#include <string>

#include <curl/curl.h>
#include "../../logger.h"

typedef enum {
    REQTYPE_UNKNOWN,

    REQTYPE_GET,
    REQTYPE_POST
} RequestType;
typedef enum {
    REQSTATE_UNKNOWN,

    REQSTATE_READY,
    REQSTATE_WORKING,
    REQSTATE_ERROR
} RequestState;
typedef enum {
    REQRES_OK,
} RequestResponseState;

class Request {
    public:
        Request(std::string m_url);

        void get();
        void post();

        static size_t writer(char *data, size_t size, size_t nmemb, std::string *writerData);
        void send();

        // event listeners
        void onFinished(std::function<void(RequestResponseState res, std::string resBody)> func);

    private:
        RequestState reqState;
        RequestType reqType;
        std::string url;
        CURL* curl;

        std::string resBody;

        std::function<void(RequestResponseState res, std::string m_resBody)> onFinishedLambda;
};