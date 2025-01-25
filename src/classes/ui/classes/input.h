#pragma once

#include <string>

#include "../../../internal/gsgl/gsgl.h"

class Input {
    public:
        Input(GSGL_Font m_font, int m_textSize, std::string m_defaultText, Color defCol, Color curCol);

        void reset();
        void update();
        void draw();

        // text functions
        void setPosition(Vector2i m_pos);
        void setSize(Vector2i m_size);
        void setRect(Vector2i m_pos, Vector2i m_size);

        void setDefaultText(std::string txt);
        std::string getText();

        void doFocus(bool state);

        // color functions

        // settings
        bool multiLine = false;
        bool resetOnFocus = false;

    private:
        std::string defaultText;
        std::string currentText;

        int selectionStart;
        int selectionSize;
        bool focus;

        GSGL_Font font;
        Color defaultTextColor;
        Color currentTextColor;

        Vector2i pos;
        Vector2i size;

        int textSize;
};