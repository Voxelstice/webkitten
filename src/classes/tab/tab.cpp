#include "tab.h"

static int currentId = 0;

Tab::Tab(std::string m_address) {
    address = m_address;
    id = currentId;
    currentId++;
}

void Tab::init() {
    //tex = LoadRenderTexture(GetScreenWidth(), GetScreenHeight() - 32);

    busy = true;
    // request networker to do things
}
void Tab::update() {
    
}

void Tab::close() {
    // we got asked to close! clear resources. and get the hell out of here
}

std::string Tab::getTitle() {
    if (title != "" && useTitle == true) {
        return title;
    } else {
        return address;
    }
}
int Tab::getId() {
    return id;
}