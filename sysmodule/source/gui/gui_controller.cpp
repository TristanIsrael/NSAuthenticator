#include "gui_controller.h"
#include "logger.h"
#include "gui/renderer.hpp"
#include "utils.h"

using namespace alefbet::authenticator::logger;
using namespace alefbet::authenticator::gfx;

/* There should only be a single transfer memory (for nv). */
alignas(ams::os::MemoryPageSize) constinit u8 g_nv_transfer_memory[0x40000];
extern "C" ::Result __nx_nv_create_tmem(TransferMemory *t, u32 *out_size, Permission perm) {
    *out_size = sizeof(g_nv_transfer_memory);
    return tmemCreateFromMemory(t, g_nv_transfer_memory, sizeof(g_nv_transfer_memory), perm);
}

void GuiController::showAuthenticationPanel() {        
    logToFile("[Gui] Show authentifcation panel\n");

    width_ = 1200; // Width must be a multiple of 64
    height_ = 720;

    u16 posX = (ScreenWidth - width_) / 2;   // Centered
    u16 posY = (ScreenHeight - height_) / 2; // Centered

    showOverlay(width_, height_, posX, posY);

    auto& renderer = Renderer::get();
    renderer.startFrame();
        
    // Draw the background
    renderer.drawRect(0, 0, width_, height_, Color(2, 4, 13, 249));

    // Draw the title
    renderer.drawString("Authentication", false, 384, 274, 62, Color(28, 194, 247, 255));
    
    renderer.endFrame();
}   

void GuiController::hideAll() {
    auto& renderer = Renderer::get();    
    if(!renderer.isInitialized()) return;

    logToFile("[Gui] Hide remaining time panel\n");
    clearScreen();
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
/*Result GuiController::hidsysEnableAppletToGetInput(bool enable, u64 aruid) {
    const struct {
        u8 permitInput;
        u64 appletResourceUserId;
    } in = { enable != 0, aruid };

    return serviceDispatchIn(hidsysGetServiceSession(), 503, in);
}*/

/*void GuiController::requestForeground(bool enabled) {
    u64 applicationAruid = 0, appletAruid = 0;

    for (u64 programId = 0x0100000000001000UL; programId < 0x0100000000001020UL; programId++) {
        pmdmntGetProcessId(&appletAruid, programId);

        if (appletAruid != 0)
            hidsysEnableAppletToGetInput(!enabled, appletAruid);
    }

    pmdmntGetApplicationProcessId(&applicationAruid);
    hidsysEnableAppletToGetInput(!enabled, applicationAruid);

    hidsysEnableAppletToGetInput(true, 0);
}*/