// Generic Software Graphics Library (GSGL)
// Designed for software rendering specifically
// Heavily inspired by raylib

// Some extras!

#include "../../logger.h"
#include "gsgl.h"

bool gsgl_IsPointInRect(Vector2i point, Vector2i pos, Vector2i size) {
    if ((point.x >= pos.x) && (point.x < (pos.x + size.x)) && (point.y >= pos.y) && (point.y < (pos.y + size.y))) return true;
    else return false;
}