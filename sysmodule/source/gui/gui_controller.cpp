#include "gui_controller.h"
#include "logger.h"
#include "gui/renderer.hpp"
#include "utils.h"
#include "helpers.h"
#include "database/database.h"
#include <mutex>

using namespace alefbet::authenticator::logger;
using namespace alefbet::authenticator::gfx;
using namespace alefbet::authenticator::helpers;
using namespace alefbet::authenticator::database;

constexpr Color textColor =         Color(0xf, 0xf, 0xf, 0xf);    // White
constexpr Color circleColor =       Color(0xf, 0xf, 0xf, 0xf);    // White
constexpr Color backgroundColor =   Color(0x2, 0x4, 0x6, 0xe);    // Deep blue
constexpr Color titleColor =        Color(0x1, 0xc, 0xe, 0xf);    // Light blue
constexpr Color errorColor =        Color(0xf, 0x0, 0x0, 0xf);    // Plain red
constexpr Color successColor =      Color(0x0, 0xf, 0xd, 0xf);    // Green

static std::mutex s_mutexVisible;

/* There should only be a single transfer memory (for nv). */
alignas(ams::os::MemoryPageSize) constinit u8 g_nv_transfer_memory[0x40000];
extern "C" ::Result __nx_nv_create_tmem(TransferMemory *t, u32 *out_size, Permission perm) {
    *out_size = sizeof(g_nv_transfer_memory);
    return tmemCreateFromMemory(t, g_nv_transfer_memory, sizeof(g_nv_transfer_memory), perm);
}

void GuiController::setVisible(bool visible) {
    std::lock_guard<std::mutex> mutex(s_mutexVisible);
    visible_ = visible;
}

bool GuiController::isVisible() const {
    std::lock_guard<std::mutex> mutex(s_mutexVisible);
    return visible_;
}

void GuiController::init() {
    logToFile("[Gui] Initialize GUI\n");
}

void GuiController::start() {
    logToFile("[Gui] starting loop\n");    

    while(true) {        
        if(isVisible()) {
            if(needsRefresh_) {                    
                refreshPanel();
                needsRefresh_ = false;
            }
            
            verifyUserInput();
        }
                
        svcSleepThread(100'000'000); // Wait 20 ms
    }

    logToFile("[Gui] loop ended\n");
}

void GuiController::showAuthenticationPanel() {        
    logToFile("[Gui] Show authentication panel\n");

    keysDown_.clear();
    enteredPin_.clear();
    user_.clear();

    // Verify whether a PIN has been set for the user
    user_ = getCurrentUser();
    auto passwords = loadPasswords();
    const auto& uid = accountUidToString(user_.uid);
    const auto& password = passwords[uid];
    
    // If there is no password for the user we need a setup
    pinStage_ = password.empty() ? PinSetup : PinVerification;
    
    width_ = 1216; // Width must be a multiple of 64
    height_ = 768;

    u16 posX = (ScreenWidth - width_) / 2;   // Centered
    u16 posY = (ScreenHeight - height_) / 2; // Centered

    showOverlay(width_, height_, posX, posY);    

    setVisible(true);

    needsRefresh_ = true;
    
    initUserInput();
    requestForeground(true);
}   

void GuiController::refreshPanel() {
    //if(!isVisible()) return;

    logToFile("[Gui] refreshing panel\n");

    auto& renderer = Renderer::get();
    renderer.startFrame();
    clearScreen(false);
        
    // Draw the background
    renderer.drawRect(0, 0, width_, height_, backgroundColor);

    // Draw the title
    renderer.drawString("Authentication", false, 384, 116, 62, titleColor);    
    
    switch(pinStage_) {
        case PinSetup: {
            // If the user does not already have a code we ask him to create one
            std::string str = user_.nickname + ", please enter a new PIN.";
            const auto& width = calculateTextWidth(str, 62);
            renderer.drawString(str.c_str(), false, (width_ - width)/2, 348, 62, textColor);            
            break;
        }
        case PinSetupVerification: {
            std::string str = "Please re-enter your PIN.";
            const auto& width = calculateTextWidth(str, 62);
            renderer.drawString(str.c_str(), false, (width_ - width)/2, 348, 62, textColor);
            break;
        }
        case PinsDontMatch: {
            std::string str = "The PINs don't match. Try again.";
            const auto& width = calculateTextWidth(str, 62);
            renderer.drawString(str.c_str(), false, (width_ - width)/2, 348, 62, errorColor);
            break;
        }
        case PinError: {
            std::string str = "Wrong PIN.";
            const auto& width = calculateTextWidth(str, 62);
            renderer.drawString(str.c_str(), false, (width_ - width)/2, 348, 62, errorColor);
            break;
        }
        case PinOk: {
            std::string str = "Correct PIN.";
            const auto& width = calculateTextWidth(str, 62);
            renderer.drawString(str.c_str(), false, (width_ - width)/2, 348, 62, successColor);
            break;
        }
        case PinVerification: {
            // Otherwise we ask the user password
            std::string str = user_.nickname + ", please enter your PIN.";
            const auto& width = calculateTextWidth(str, 62);
            renderer.drawString(str.c_str(), false, (width_ - width)/2, 348, 62, textColor);  
            break;  
        }
    }
    
    // Draw the circles
    renderer.drawCircle(416, 496, 24, keysDown_.size() >= 1, circleColor);
    renderer.drawCircle(550, 496, 24, keysDown_.size() >= 2, circleColor);
    renderer.drawCircle(680, 496, 24, keysDown_.size() >= 3, circleColor);
    renderer.drawCircle(818, 496, 24, keysDown_.size() > 3, circleColor);

    renderer.endFrame();    
}

int GuiController::calculateTextWidth(const std::string& text, int fontSize, bool monospace)
{
    auto& renderer = Renderer::get();
    const auto& dimension = renderer.drawString(text.c_str(), monospace, 0, 100, fontSize, Color(0, 0, 0, 0));
    return dimension.first;
}

void GuiController::hideAll() {
    setVisible(false);

    auto& renderer = Renderer::get();    
    if(!renderer.isInitialized()) return;

    requestForeground(false);

    logToFile("[Gui] Hide remaining time panel\n");
    clearScreen();

    // We need to free all video resources
    renderer.exit();    

    hidsysExit();
    hidExit();    
}

void GuiController::showOverlay(u16 width, u16 height, u16 posX, u16 posY) {
    logToFile("[Gui] show Overlay of size %ix%i\n", width, height);

    auto& renderer = Renderer::get();    
    renderer.init(width, height, posX, posY);
    clearScreen();
}

void GuiController::clearScreen(bool ownFrame) {
    auto& renderer = Renderer::get();

    if(ownFrame) {
        renderer.startFrame();
    }

    renderer.clearScreen();
    
    if(ownFrame) {
        renderer.endFrame();
    }
}

void GuiController::handlePinInput() {
    // When this function is called it means that a PIN has been entered
    // There are 2 possibilities:
    // - This is the new PIN and we have to ask the user to re-enter for verification
    // - This is the control PIN and we have to verify it
    if(pinStage_ == PinSetup || pinStage_ == PinsDontMatch) {
        enteredPin_ = encodePassword(keysDown_);
        keysDown_.clear();
        pinStage_ = PinSetupVerification;
        needsRefresh_ = true;
    } else if(pinStage_ == PinSetupVerification) {
        const auto& verifPin = encodePassword(keysDown_);
        keysDown_.clear();
        
        pinStage_ = verifPin == enteredPin_ ? PinOk : PinsDontMatch;

        if(pinStage_ == PinOk) {
            const auto& uid = accountUidToString(user_.uid);
            savePassword(uid, verifPin);
        }
        
        needsRefresh_ = true;
    }
 }

/**
 * @brief libnx hid:sys shim that gives or takes away frocus to or from the process with the given aruid
 *
 * @param enable Give focus or take focus
 * @param aruid Aruid of the process to focus/unfocus
 * @return Result Result
 */
Result GuiController::hidsysEnableAppletToGetInput(bool enable, u64 aruid) {
    const struct {
        u8 permitInput;
        u64 appletResourceUserId;
    } in = { enable != 0, aruid };

    return serviceDispatchIn(hidsysGetServiceSession(), 503, in);
}

void GuiController::requestForeground(bool enabled) {
    u64 applicationAruid = 0, appletAruid = 0;

    //logToFile("[Gui] Request foreground\n");
    Result rc = 0;
    for (u64 programId = 0x0100000000001000UL; programId < 0x0100000000001020UL; programId++) {
        rc = pmdmntGetProcessId(&appletAruid, programId);
        //logToFile("[Gui] programId=%i, appletAruid=%i, result=%i:%i\n", programId, appletAruid, R_MODULE(rc), R_DESCRIPTION(rc));

        if (appletAruid != 0) {
            rc = hidsysEnableAppletToGetInput(!enabled, appletAruid);
            //logToFile("[Gui] hidsysEnableAppletToGetInput -> false, result=%i:%i\n", R_MODULE(rc), R_DESCRIPTION(rc));
        }
    }

    rc = pmdmntGetApplicationProcessId(&applicationAruid);
    //logToFile("[Gui] pmdmntGetApplicationProcessId, applicationAruid=%i, result=%i:%i\n", applicationAruid, R_MODULE(rc), R_DESCRIPTION(rc));
    rc = hidsysEnableAppletToGetInput(!enabled, applicationAruid);
    //logToFile("[Gui] hidsysEnableAppletToGetInput -> false, applicationAruid=%i, result=%i:%i\n", applicationAruid, R_MODULE(rc), R_DESCRIPTION(rc));

    rc = hidsysEnableAppletToGetInput(true, 0);
    //logToFile("[Gui] hidsysEnableAppletToGetInput -> true (0), result=%i:%i\n", R_MODULE(rc), R_DESCRIPTION(rc));
}

void GuiController::initUserInput() {
    logToFile("[Gui] Initialize user input\n");

    hidInitialize();
    hidsysInitialize();

    // Allow only Player 1 and handheld mode
    HidNpadIdType id_list[2] = { HidNpadIdType_No1, HidNpadIdType_Handheld };
    
    // Configure HID system to only listen to these IDs
    hidSetSupportedNpadIdType(id_list, 2);
    
    // Configure input for up to 2 supported controllers (P1 + Handheld)
    padConfigureInput(2, HidNpadStyleSet_NpadStandard | HidNpadStyleTag_NpadSystemExt);
    
    // Initialize separate pad states for both controllers    
    padInitialize(&pad_p1_, HidNpadIdType_No1);
    padInitialize(&pad_handheld_, HidNpadIdType_Handheld);
    
    // Touch screen init
    //hidInitializeTouchScreen();

    // Clear any stale input from both controllers
    padUpdate(&pad_p1_);
    padUpdate(&pad_handheld_);
}

void GuiController::verifyUserInput() {
    if(!visible_) return;  
    
    padUpdate(&pad_p1_);
    padUpdate(&pad_handheld_);

    const u64 kDown_p1 = padGetButtonsDown(&pad_p1_);
    const u64 kDown_handheld = padGetButtonsDown(&pad_handheld_);

    u64 keysDown = kDown_p1 | kDown_handheld;

    if(keysDown != 0) {
        logToFile("[Gui] keysDown=%i\n", keysDown);
        keysDown_.push_back(keysDown); 

        if(keysDown_.size() == 4) {
            handlePinInput();
        }

        needsRefresh_ = true;
    }
}