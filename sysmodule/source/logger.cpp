#include "logger.h"
#include <mutex>
#include <cstdarg>
#include <cstring>
#include <cassert>
#include <switch.h>
#include "utils.h"

constexpr const char* LogFilename = "/atmosphere/logs/authenticator_svc.log";

namespace alefbet {
    namespace authenticator {

        namespace {
            static std::mutex s_mutex;                
            static constinit char g_format_buffer[2 * ams::os::MemoryPageSize];
            static FsFileSystem sdmc;
            static bool ready = false;
            static FsFile handle;
            static size_t offset = 0;
            static bool opened = false;    
        }

        typedef struct HipcMetadata {
            u32 type;
            u32 num_send_statics;
            u32 num_send_buffers;
            u32 num_recv_buffers;
            u32 num_exch_buffers;
            u32 num_data_words;
            u32 num_recv_statics; // also accepts HIPC_AUTO_RECV_STATIC
            u32 send_pid;
            u32 num_copy_handles;
            u32 num_move_handles;
        } HipcMetadata;

        namespace logger {           
            
            bool prepare() {
                if(ready) return true;

                ready = R_SUCCEEDED(fsOpenSdCardFileSystem(&sdmc));

                return ready;
            }

            bool openFile() {
                if(opened) return true;
                if(!prepare()) return false;

                // Try to open the file
                opened = R_SUCCEEDED(fsFsOpenFile(&sdmc, LogFilename, FsOpenMode_Write | FsOpenMode_Append, &handle));

                // If the file could not be opened 
                if(!opened) {
                    // try to create a new one
                    if(R_SUCCEEDED(fsFsCreateFile(&sdmc, LogFilename, 0, 0))) {
                        // Then open it
                        opened = R_SUCCEEDED(fsFsOpenFile(&sdmc, LogFilename, FsOpenMode_Write | FsOpenMode_Append, &handle));
                    }
                }                 
                
                return opened;
            }
            
            void clearLog() {
                if(opened) return;
                if(prepare()) {
                    fsFsDeleteFile(&sdmc, LogFilename);
                }
            }          

            void logToFile(const char* fmt, ...) {
                std::lock_guard<std::mutex> lock(s_mutex);

                if(openFile()) {
                    std::va_list vl;
                    va_start(vl, fmt);
                    vsnprintf(g_format_buffer, sizeof(g_format_buffer), fmt, vl);
                    va_end(vl);    
                    
                    size_t size = std::strlen(g_format_buffer);
                    if(R_SUCCEEDED(fsFileWrite(&handle, offset, g_format_buffer, size, FsWriteOption_Flush))) {
                        offset += size;                        
                    }

                    closeFile();
                }                        
                
            }

            void debugHipcMetaHeader(void* hdr) {
                HipcMetadata* h = (HipcMetadata*)hdr;

                logToFile("HipcHeader contents:\n");
                logToFile("  type               : %u\n", h->type);
                logToFile("  num_send_statics   : %u\n", h->num_send_statics);
                logToFile("  num_send_buffers   : %u\n", h->num_send_buffers);
                logToFile("  num_recv_buffers   : %u\n", h->num_recv_buffers);
                logToFile("  num_exch_buffers   : %u\n", h->num_exch_buffers);
                logToFile("  num_data_words     : %u\n", h->num_data_words);
                logToFile("  num_recv_statics   : %u\n", h->num_recv_statics);
                logToFile("  send_pid           : %u\n", h->send_pid);
                logToFile("  num_copy_handles   : %u\n", h->num_copy_handles);
                logToFile("  num_move_handles   : %u\n", h->num_move_handles);
            }

            void closeFile() {
                if(opened) {
                    fsFileClose(&handle);
                    opened = false;
                }
            }

        }
    }

}