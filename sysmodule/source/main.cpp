#include <switch.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include "logger.h"
#include "utils.h"
#include "monitor.h"

using namespace alefbet::authenticator::logger;

#ifdef __cplusplus
extern "C" {
#endif

    constexpr size_t TotalHeapSize = ams::util::AlignUp(1_MB, ams::os::MemoryHeapUnitSize);

    constexpr size_t ThreadMonitorStackRequiredSizeBytes = ams::util::AlignUp(256_KB, 128);
    constexpr size_t ThreadMonitorStackRequiredSizeAligned = ams::util::AlignUp(ThreadMonitorStackRequiredSizeBytes, ams::os::MemoryPageSize);
    
    alignas(ams::os::MemoryPageSize) constinit u8 g_thread_monitor_memory[ThreadMonitorStackRequiredSizeAligned];

    // Minimize fs resource usage
    u32 __nx_fsdev_direntry_cache_size = 1;
    bool __nx_fsdev_support_cwd = false;

    u32 __nx_applet_type = AppletType_None;
    ViLayerFlags __nx_vi_stray_layer_flags = (ViLayerFlags)0;

    void __libnx_initheap(void)
    {
        extern char* fake_heap_start;
        extern char* fake_heap_end;        
        void* addr = nullptr;

        Result rc = svcSetHeapSize(&addr, TotalHeapSize);
        if(R_SUCCEEDED(rc)) {
            fake_heap_start = (char*)addr;
            fake_heap_end   = fake_heap_start + TotalHeapSize;
        }
    }

    void __appInit(void)
    {
        Result rc;

        rc = smInitialize();
        if (R_FAILED(rc))
            fatalThrow(MAKERESULT(Module_HomebrewLoader, 1));

        rc = setsysInitialize();
        if (R_SUCCEEDED(rc)) {
            SetSysFirmwareVersion fw;
            rc = setsysGetFirmwareVersion(&fw);
            if (R_SUCCEEDED(rc))
                hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
            setsysExit();
        }

        rc = fsInitialize();
        if (R_FAILED(rc))
            fatalThrow(MAKERESULT(Module_HomebrewLoader, 2));
                                        
        pmdmntInitialize();
        nsInitialize();        
    }

    void __wrap_exit(void)
    {
        smExit(); 
        svcExitProcess();        
        __builtin_unreachable();
    }

    void testMemory() {
        int on_stack = 0;
        int* on_heap = new int(0);

        extern char* fake_heap_start;
        extern char* fake_heap_end;

        //Memory allocation test
        /*for(int s = 0x1000 ; s <= 0x150000 ; s += 5000) {
            void* ptr = aligned_alloc(0x1000, s);
            if(ptr != nullptr) {
                logToFile("Allocation of %i bytes succeeded. @ptr=%p\n", s, (void*)ptr);
                free(ptr);
            } else {
                logToFile("Allocation of %i bytes failed\n", s);    
                break;                                            
            }            
        } */
    }
}

namespace alefbet::authenticator {

    void startMonitor(void* args) {
        alefbet::authenticator::srv::Monitor* monitor = new alefbet::authenticator::srv::Monitor;
        logToFile("@monitor=%p\n", monitor);

        // Start monitoring
        monitor->start();

        // Start loop
        monitor->loop();
    }

}

int main(int argc, char **argv)
{    
    if (hosversionBefore(9,0,0))
        exit(1);

    clearLog();
    logToFile("[Main] Authenticator starting\n");

    //testMemory();

    ::Result rc = 0;    

    // Start the monitor        
    Thread threadMonitor;    
    rc = threadCreate(&threadMonitor, alefbet::authenticator::startMonitor, NULL, g_thread_monitor_memory, ThreadMonitorStackRequiredSizeAligned, 0x2c, -2);
    if(R_FAILED(rc)) {
        logToFile("Could not create the monitor thread, error %i:%i.\n", R_MODULE(rc), R_DESCRIPTION(rc));
        return 4;
    }
    rc = threadStart(&threadMonitor); // Run the monitor's loop
    if(R_FAILED(rc)) {
        logToFile("Could not start the monitor thread, error %i:%i.\n", R_MODULE(rc), R_DESCRIPTION(rc));
        return 5;
    }
    
    rc = threadWaitForExit(&threadMonitor);
    if(R_FAILED(rc)) {
        logToFile("Could not wait for the monitor thread to end, error %i:%i.\n", R_MODULE(rc), R_DESCRIPTION(rc));
        return 7;
    }

    logToFile("[Main] Authenticator ended\n");

    return 0;
}
