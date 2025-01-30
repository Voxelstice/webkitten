#pragma once

#include "../main/request.h"

#include <string>
#include <vector>

class Tab {
    public:
        Tab(std::string m_address);

        void init();
        void update();
        void draw();

        void close();

        std::string getTitle();
        std::string getAddress();
        int getId();

        void setFocusState(bool state);

        //RenderTexture2D tex;
    private:
        bool focused = false;
        bool busy = false;
        bool useTitle = false;

        std::string title = "";
        std::string address = "";
        std::string requestResult = "There's nothing here buddy";
        int id = -1;

        Request *testReq;
        std::vector<Request> requestQueue;
};