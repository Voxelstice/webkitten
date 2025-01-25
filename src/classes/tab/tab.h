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
        int getId();

        //RenderTexture2D tex;
    private:
        bool busy = false;
        bool useTitle = false;

        std::string title = "";
        std::string address = "";
        int id = -1;

        std::vector<Request> requestQueue;
};