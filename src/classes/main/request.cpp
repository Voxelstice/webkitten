#include "request.h"
#include "../../main.h"
#include "../../logger.h"
#include <string>

Request::Request(std::string m_url) {
    url = m_url;
    reqType = REQTYPE_UNKNOWN;
    reqState = REQSTATE_UNKNOWN;

    resBody = "";

    if (!networker->IsReady()) {
        reqState = REQSTATE_ERROR;
        Logger_log(LOGGER_ERROR, "NETWORK: Attempt to create a request while networking is not initialized");
        throw "Networking is not initialized";
    }

    curl = curl_easy_init();
    if (!curl) {
        reqState = REQSTATE_ERROR;
        Logger_log(LOGGER_ERROR, "NETWORK: Request curl instance initialization failed");
        throw "curl instance initialization failed";
    } else {
        networker->SetInstanceDef(curl);
        curl_easy_setopt(curl, CURLOPT_URL, m_url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resBody);

        reqState = REQSTATE_READY;
    }
}

size_t Request::writer(char *data, size_t size, size_t nmemb, std::string *writerData) {
  if(writerData == NULL)
    return 0;
 
  writerData->append(data, size*nmemb);
 
  return size * nmemb;
}

void Request::send() {
    if (reqState != REQSTATE_READY) return;

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        onFinishedLambda(REQRES_OK, resBody);
    } else {
        // handle it later
    }
}

void Request::get() {
    reqType = REQTYPE_GET;
}
void Request::post() {
    reqType = REQTYPE_POST;
}

void Request::onFinished(std::function<void(RequestResponseState res, std::string m_resBody)> func) {
    onFinishedLambda = func;
}