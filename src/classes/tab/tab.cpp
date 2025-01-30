#include "tab.h"

#include "../../internal/gsgl/gsgl.h"
#include "../ui/fonts.h"

static int currentId = 0;

Tab::Tab(std::string m_address) {
    address = m_address;
    id = currentId;
    currentId++;
    busy = false;
}

void Tab::init() {
    testReq = new Request(address);

    auto onFinished = [this](RequestResponseState res, std::string m_resBody){
        requestResult = m_resBody;
        //printf("%s\n", requestResult.c_str());
    };

    testReq->onFinished(onFinished);
}
void Tab::update() {
    if (focused == true && busy == false) {
        busy = true;
        testReq->get();
        testReq->send();
    }
}
void Tab::draw() {
    gsgl_DrawText(GetFont(PROGGY_CLEAN), requestResult.c_str(), 16, 80, 16, {255, 255, 255, 255});
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
std::string Tab::getAddress() {
    return address;
}
int Tab::getId() {
    return id;
}

void Tab::setFocusState(bool state) {
    focused = state;
}