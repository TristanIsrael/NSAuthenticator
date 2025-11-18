#include "monitor.h"
#include "logger.h"
#include "literals.h"
#include "helpers.h"
#include <chrono>

using namespace std::chrono;
using namespace alefbet::authenticator::logger;
using namespace alefbet::authenticator::helpers;

constexpr seconds MainLoopDelayInSeconds = 30s;
constexpr nanoseconds MainLoopDelayInNanos = duration_cast<nanoseconds>(MainLoopDelayInSeconds);

namespace alefbet::authenticator::srv {

    void Monitor::start() {
        if(running_) return;
        running_ = true;
    }

    /*!
        \brief Monitors running game and show the authentication window if a new game has been started.

        The monitoring is done by looking at the currently running game and compare with the previous value.
    */
    void Monitor::loop() {        
        logToFile("[Monitor] Starting monitoring loop\n");

        currentTitle_ = 0;
        currentUser_ = UserData{};

        while(true) {
            if(!running_) {
                //Do nothing
                svcSleepThread(1000*500); // Wait for 500ms
                continue;
            }
            
            logToFile("[Monitor] Monitoring loop has started\n");

            const auto& currentAppPid = getRunningApplicationPid();
            const auto& currentTitle = getRunningApplicationTitleId(currentAppPid);
            const auto& currentUser = getCurrentUser();            

            if(currentTitle != currentTitle_ && currentUser != currentUser_) {
                currentTitle_ = currentTitle;
                currentUser_ = currentUser;

                // Verify whether a game has been closed
                if(currentTitle == 0) {
                    logToFile("[Monitor] The game has been closed");
                    
                } else { // Otherwise react
                    logToFile("[Monitor] The current game and/or user has changed\n");

                }
            }
            
            logToFile("[Monitoring] loop\n");
            svcSleepThread(MainLoopDelayInNanos.count()); //Wait
        }

        logToFile("[Monitor] Stopped monitoring.\n");
    }

    void Monitor::stop() {
        logToFile("[Monitor] Stopping monitor\n");
        running_ = false;
    }

}