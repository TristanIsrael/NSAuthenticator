#pragma once
#include <switch.h>
#include "helpers.h"
#include "gui/gui_controller.h"

using namespace alefbet::authenticator::structs;

namespace alefbet::authenticator::srv {

    /*! \brief The monitor class is used to monitor applications usage and update the database records.
    */
    class Monitor {
        public:         
            Monitor(GuiController* gui) 
            : guiController_(gui) {}
            void start();
            void stop();
            void loop();
            bool isRunning() const {
                return running_;
            } 

        private:
            void handleRunningApp(u64 pid);
            GuiController* getGuiController();

        private:
            bool running_ = false;
            u64 currentTitle_ = 0;
            UserData currentUser_;
            bool firstStart_ = true;
            GuiController* guiController_ = nullptr;
    };    

};