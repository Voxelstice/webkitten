// This is the border of the networker. Authorized personnel only beyond this point.
// The comment above is a joke.

// This is meant to isolate libcurl's windows.h inclusion since it seems to break all of raylib when used together.

#include "networker.h"

Networker* networker;

void networker_init() {
    networker = new Networker();
    networker->init();
}