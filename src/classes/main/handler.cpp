#include "handler.h"

#include "../ui/fonts.h"

Handler::Handler() {
    // do nothing
}

void Handler::init() {
    tabs.clear();
    ready = true;

    Tab* tab = new Tab("https://www.example.com/");
    tab->init();
    tabs.push_back(tab);

    Tab* tab1 = new Tab("https://en.wikipedia.org/wiki/CFM_International_CFM56/");
    tab1->init();
    tabs.push_back(tab1);

    Tab* tab2 = new Tab("https://github.com/Voxelstice/tinyweb");
    tab2->init();
    tabs.push_back(tab2);

    Tab* tab3 = new Tab("https://www.youtube.com/");
    tab3->init();
    tabs.push_back(tab3);

    Tab* tab4 = new Tab("https://www.google.com/");
    tab4->init();
    tabs.push_back(tab4);

    Tab* tab5 = new Tab("https://www.geeksforgeeks.org/stdstringinsert-in-c/");
    tab5->init();
    tabs.push_back(tab5);

    Tab* tab6 = new Tab("https://stackoverflow.com/questions/14762456/getclipboarddatacf-text");
    tab6->init();
    tabs.push_back(tab6);

    Tab* tab7 = new Tab("https://learn.microsoft.com/en-us/windows/win32/dataxchg/using-the-clipboard#copying-information-to-the-clipboard");
    tab7->init();
    tabs.push_back(tab7);

    // init input
    input = new Input(GetFont(PROGGY_CLEAN), 24, "Enter web address", {160, 160, 160, 255}, {255, 255, 255, 255});
}
void Handler::update() {
    input->update();
    for (int i = 0; i < tabs.size(); i++) {
        tabs[i]->update();
    }
}
void Handler::draw() {
    int tabId = getTab(tabFocus);
    if (tabId != -1) {
        tabs[tabId]->draw();
    }
}

void Handler::focusTab(int id) {
    for (int i = 0; i < tabs.size(); i++) {
        if (tabs[i]->getId() == id) {
            tabs[i]->setFocusState(true);
            input->setText(tabs[i]->getAddress());
            tabFocus = id;
            return;
        } else {
            tabs[i]->setFocusState(false);
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
int Handler::getTab(int id) {
    for (int i = 0; i < tabs.size(); i++) {
        if (tabs[i]->getId() == id) {
            return i;
        }
    }
    return -1;
}

void Handler::drawInput(Vector2i pos, Vector2i size) {
    input->setRect(pos, size);
    input->draw();
}