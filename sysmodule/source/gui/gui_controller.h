#pragma once
#include <switch.h>
#include <string>
#include <vector>
#include "helpers.h"

using namespace alefbet::authenticator::structs;

class GuiController {
    public:
        void init();
        void start();
        void showAuthenticationPanel();
        void hideAll();

    private:
        void showOverlay(u16 width, u16 height, u16 posX, u16 posY);
        void clearScreen(bool ownFrame = true);
        void refreshPanel();
        int calculateTextWidth(const std::string& text, int fontSize, bool monospace = false);
        Result hidsysEnableAppletToGetInput(bool enable, u64 aruid);
        void requestForeground(bool enabled);

        void initUserInput();
        void verifyUserInput();

        void handlePinInput();

        void setVisible(bool visible);
        bool isVisible() const;

        typedef enum {
            PinSetup,
            PinSetupVerification,
            PinVerification,
            PinsDontMatch,
            PinOk,
            PinError
        } PinStage;

    private:
        u16 width_ = 0;
        u16 height_ = 0;
        bool visible_ = false;
        bool needsRefresh_ = false;
        PadState pad_p1_;
        PadState pad_handheld_;
        std::vector<u64> keysDown_;
        PinStage pinStage_ = PinSetup;
        std::string enteredPin_;
        UserData user_;
};
