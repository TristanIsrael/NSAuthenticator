#include "monitor.h"
#include "logger.h"
#include "literals.h"
#include "helpers.h"
#include <chrono>

using namespace std::chrono;
using namespace alefbet::authenticator::logger;
using namespace alefbet::authenticator::helpers;

constexpr s64 MainLoopDelayInNanos = 30'000'000'000; // 30 seconds

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

        logToFile("[Monitor] Monitoring loop has started\n");

        while(true) {
            logToFile("[Monitor] begin loop\n");

            if(!running_) {
                // Do nothing
                svcSleepThread(500'000'000); // Wait for 500 ms
                continue;
            }                    

            if(firstStart_) {
                // On the first start we wait for 30 seconds
                svcSleepThread(MainLoopDelayInNanos);
                firstStart_ = false;
            }

            const auto& currentAppPid = getRunningApplicationPid();
            if(currentAppPid != 0) {
                handleRunningApp(currentAppPid);
            } else {
                // The app has been closed?
                if(currentTitle_ > 0) {
                    logToFile("[Monitor] The game has been closed");
                    currentTitle_ = 0;
                    currentUser_.clear();
                }
            }
                          
            svcSleepThread(MainLoopDelayInNanos); //Wait            
        }

        logToFile("[Monitor] Stopped monitoring.\n");
    }

    void Monitor::stop() {
        logToFile("[Monitor] Stopping monitor\n");
        running_ = false;
    }

    void Monitor::handleRunningApp(u64 pid) {
        logToFile("[Monitor] handle running app %i\n", pid);

        const auto& currentTitle = getRunningApplicationTitleId(pid);
        const auto& currentUser = getCurrentUser();

        if(currentTitle != currentTitle_ && currentUser != currentUser_) {
            currentTitle_ = currentTitle;
            currentUser_ = currentUser;

            logToFile("[Monitor] The current game and/or user has changed\n");
            
        }
    }
}