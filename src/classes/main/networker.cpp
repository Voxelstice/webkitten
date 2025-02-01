#include "networker.h"
#include <string>

Networker::Networker() {
    ready = false;
}

void Networker::init() {
    // do basic global initialization first
    Logger_logI("NETWORK: Initializing libcurl");

    CheckCode(curl_global_init(CURL_GLOBAL_DEFAULT));

    // Attempt to do a version print if possible
    curl_version_info_data *ver;

    ver = curl_version_info(CURLVERSION_NOW);

    bool httpPresent = false;
    bool httpsPresent = false;
    if (ver) {
        Logger_logI("NETWORK: --- Begin libcurl version info ---");

        Logger_logI("NETWORK: - libcurl: %s", ver->version);
        Logger_logI("NETWORK: - ssl: %s", ver->ssl_version);

        const char *const *ptr;

        // get protocols
        std::string protocols = "";
        for (ptr = ver->protocols; *ptr; ++ptr)
            protocols = protocols + *ptr + " ";
        Logger_logI("NETWORK: - protocols: %s", protocols.c_str());

        if (protocols.find("http")) httpPresent = true;
        if (protocols.find("https")) httpsPresent = true;

        // get features
        std::string features = "";
        for (ptr = ver->feature_names; *ptr; ++ptr)
            features = features + *ptr + " ";
        Logger_logI("NETWORK: - features: %s", features.c_str());

        Logger_logI("NETWORK: --- End libcurl version info ---");
    } else {
        Logger_logI("NETWORK: Cannot do libcurl version info print");
    }

    if (httpPresent == false) {
        Logger_log(Logger_logType(2), "NETWORK: HTTP is not present!!!!");
    } else {
        if (httpsPresent == false) {
            Logger_log(Logger_logType(1), "NETWORK: HTTP is present, HTTPS is not present");
        } else {
            Logger_log(Logger_logType(0), "NETWORK: HTTP is present, HTTPS is present");
        }
        ready = true;
    }

    Logger_log(LOGGER_INFO, "----------------------------------------------------------------------------------");
}
void Networker::update() {
    
}

void Networker::CheckCode(CURLcode code) {
    if (code != CURLE_OK) {
        Logger_logE("NETWORK: libcurl error: %s", curl_easy_strerror(code));
        std::string str = "libcurl error: ";
        str = str + (curl_easy_strerror(code));
        #ifdef _WIN32
        MessageBox(0, str.c_str(), "tinyweb", 0x00000010L);
        #else

        #endif
    }
}
bool Networker::IsReady() {
    return ready;
}

// Set default properties for a curl instance
void Networker::SetInstanceDef(CURL* curl) {
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "tinyweb/1.0");

    SetInstanceCert(curl);
}
// Set certificate properties for a curl instance
void Networker::SetInstanceCert(CURL* curl) {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 1);
    curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
    curl_easy_setopt(curl, CURLOPT_CAPATH, "cacert.pem");

    curl_easy_setopt(curl, CURLOPT_CA_CACHE_TIMEOUT, 604800L);
}

/*#include <curl/curl.h>
int main2(void) {
    CURL *curl;
    CURLcode res;
    curl_version_info_data *ver;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    ver = curl_version_info(CURLVERSION_NOW);
    if (ver->features & CURL_VERSION_HTTP2)
        printf("HTTP/2 support is present\n");

    if (ver->features & CURL_VERSION_HTTP3)
        printf("HTTP/3 support is present\n");

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://example.com/");

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 1);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
        curl_easy_setopt(curl, CURLOPT_CAPATH, "cacert.pem");

        curl_easy_setopt(curl, CURLOPT_CA_CACHE_TIMEOUT, 604800L);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    return 0;
}*/