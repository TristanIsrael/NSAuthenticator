#pragma once
#include <switch.h>

class GuiController {
    public:
        void showAuthenticationPanel();
        void hideAll();

    private:
        void showOverlay(u16 width, u16 height, u16 posX, u16 posY);
        void clearScreen();
        
    private:
        u16 width_ = 0;
        u16 height_ = 0;
};
