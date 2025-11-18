#pragma once
#include <string>

namespace alefbet {
    namespace authenticator { 
        namespace logger {
            bool openFile();
            void clearLog();
            void logToFile(const char *fmt, ...);
            void debugHipcMetaHeader(void* hdr);
            //void debugHipcDataHeader(void* hdr);
            void closeFile();
        }
    }
}

