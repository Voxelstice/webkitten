#include "handler.h"

Handler::Handler() {
    // do nothing
}

void Handler::init() {
    tabs.clear();
    ready = true;

    Tab* tab = new Tab("https://www.example.com/");
    tab->init();
    tabs.push_back(tab);

    Tab* cfm = new Tab("https://en.wikipedia.org/wiki/CFM_International_CFM56/");
    cfm->init();
    tabs.push_back(cfm);
}
void Handler::update() {
    
}

void Handler::focusTab(int id) {
    for (int i = 0; i < tabs.size(); i++) {
        if (tabs[i]->getId() == id) {
            tabFocus = id;
            return;
        }
    }
    tabFocus = -1;
}
void Handler::closeTab(int id) {
    for (int i = 0; i < tabs.size(); i++) {
        if (tabs[i]->getId() == id) {
            tabs[i]->close();
            tabs.erase(tabs.begin() + i);
        }
    }
}