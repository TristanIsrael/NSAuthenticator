#include "gui_controller.h"
#include "logger.h"
#include "gui/renderer.hpp"
#include "utils.h"
#include "helpers.h"
#include "database/database.h"

using namespace alefbet::authenticator::logger;
using namespace alefbet::authenticator::gfx;
using namespace alefbet::authenticator::helpers;
using namespace alefbet::authenticator::database;

constexpr Color textColor =         Color(0xf, 0xf, 0xf, 0xf);    // White
constexpr Color circleColor =       Color(0xf, 0xf, 0xf, 0xf);    // White
constexpr Color backgroundColor =   Color(0x2, 0x4, 0x6, 0xe);    // Deep blue
constexpr Color titleColor =        Color(0x1, 0xc, 0xe, 0xf);    // Light blue

/* There should only be a single transfer memory (for nv). */
alignas(ams::os::MemoryPageSize) constinit u8 g_nv_transfer_memory[0x40000];
extern "C" ::Result __nx_nv_create_tmem(TransferMemory *t, u32 *out_size, Permission perm) {
    *out_size = sizeof(g_nv_transfer_memory);
    return tmemCreateFromMemory(t, g_nv_transfer_memory, sizeof(g_nv_transfer_memory), perm);
}

void GuiController::init() {
    logToFile("[Gui] Initialize GUI\n");
}

void GuiController::start() {
    logToFile("[Gui] starting loop\n");

    while(true) {
        if(visible_) {
            
            if(needsRefresh_) {
                refreshPanel();
                needsRefresh_ = false;
            }
            
            verifyUserInput();
        }
        
        svcSleepThread(100'000'000); // Wait 100 ms
    }

    logToFile("[Gui] loop ended\n");
}

void GuiController::showAuthenticationPanel() {        
    logToFile("[Gui] Show authentication panel\n");

    width_ = 1216; // Width must be a multiple of 64
    height_ = 768;

    u16 posX = (ScreenWidth - width_) / 2;   // Centered
    u16 posY = (ScreenHeight - height_) / 2; // Centered

    showOverlay(width_, height_, posX, posY);    

    visible_ = true;
    needsRefresh_ = true;

    initUserInput();
    requestForeground(true);
}   

void GuiController::refreshPanel() {
    logToFile("[Gui] refreshing panel\n");

    auto& renderer = Renderer::get();
    renderer.startFrame();
        
    // Draw the background
    renderer.drawRect(0, 0, width_, height_, backgroundColor);

    // Draw the title
    renderer.drawString("Authentication", false, 384, 116, 62, titleColor);

    // Get the user current password
    const auto& passwords = loadPasswords();

    const auto& user = getCurrentUser();
    const auto& uidStr = accountUidToString(user.uid);
    
    if(!passwords.contains(uidStr)) {
        // If the user does not already have a code we ask him to create one
        std::string str = user.nickname + ", please enter a new PIN.";
        const auto& width = calculateTextWidth(str, 62);
        renderer.drawString(str.c_str(), false, (width_ - width)/2, 348, 62, textColor);
    } else {
        // Otherwise we ask the user password
        std::string str = user.nickname + ", please enter your PIN.";
        const auto& width = calculateTextWidth(str, 62);
        renderer.drawString(str.c_str(), false, (width_ - width)/2, 348, 62, textColor);
    }
    
    // Draw the circles
    renderer.drawCircle(416, 496, 24, false, circleColor);
    renderer.drawCircle(550, 496, 24, false, circleColor);
    renderer.drawCircle(680, 496, 24, false, circleColor);
    renderer.drawCircle(818, 496, 24, false, circleColor);

    renderer.endFrame();    
}

int GuiController::calculateTextWidth(const std::string& text, int fontSize, bool monospace)
{
    auto& renderer = Renderer::get();
    const auto& dimension = renderer.drawString(text.c_str(), monospace, 0, 100, fontSize, Color(0, 0, 0, 0));
    return dimension.first;
}

void GuiController::hideAll() {
    auto& renderer = Renderer::get();    
    if(!renderer.isInitialized()) return;

    logToFile("[Gui] Hide remaining time panel\n");
    clearScreen();

    // We need to free all video resources
    renderer.exit();
    visible_ = false;
}

void GuiController::showOverlay(u16 width, u16 height, u16 posX, u16 posY) {
    logToFile("[Gui] show Overlay of size %ix%i\n", width, height);

    auto& renderer = Renderer::get();    
    renderer.init(width, height, posX, posY);
    clearScreen();
}

void GuiController::clearScreen() {
    auto& renderer = Renderer::get();
    renderer.startFrame();
    renderer.clearScreen();
    renderer.endFrame();
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

    for (u64 programId = 0x0100000000001000UL; programId < 0x0100000000001020UL; programId++) {
        pmdmntGetProcessId(&appletAruid, programId);

        if (appletAruid != 0)
            hidsysEnableAppletToGetInput(!enabled, appletAruid);
    }

    pmdmntGetApplicationProcessId(&applicationAruid);
    hidsysEnableAppletToGetInput(!enabled, applicationAruid);

    hidsysEnableAppletToGetInput(true, 0);
}

void GuiController::initUserInput() {
    logToFile("[Gui] Initialize user input\n");

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
    // hidInitializeTouchScreen();
    
    // Clear any stale input from both controllers
    padUpdate(&pad_p1_);
    padUpdate(&pad_handheld_);
}

void GuiController::verifyUserInput() {
    padUpdate(&pad_p1_);
    padUpdate(&pad_handheld_);

    const u64 kDown_p1 = padGetButtonsDown(&pad_p1_);
    const u64 kDown_handheld = padGetButtonsDown(&pad_handheld_);

    u64 keysDown = kDown_p1 | kDown_handheld;

    if(keysDown != 0) {
        logToFile("[Gui] keysDown=%i\n", keysDown);
    }
}