#include <fstream>
#include <sys/stat.h>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <switch.h>
#include <filesystem>
#include <mutex>
#include "database.h"
#include "json.hpp"
#include "logger.h"

using json = nlohmann::json;
using namespace alefbet::authenticator::logger;
using namespace alefbet::authenticator::helpers;

static const char* DATA_DIR = "/config/authenticator";
static const char* DB_FILENAME = "/config/authenticator/passwords.json";

namespace alefbet::authenticator::database {
    static FsFileSystem sdmc;
    static FsFile handle_database;
    static std::mutex mutex_database;
    static bool ready = false;

    bool prepare() {
        if(ready) return true;

        ready = R_SUCCEEDED(fsOpenSdCardFileSystem(&sdmc));
        if(!ready) {
            logToFile("[Database] Could not get access to SD card\n");
        }

        return ready;
    }

    bool createDataDirectory() 
    {
        if(!prepare()) return false;

        bool result = R_SUCCEEDED(fsFsCreateDirectory(&sdmc, DATA_DIR));

        return result;
    }

    Passwords loadPasswords() 
    {
        std::lock_guard<std::mutex> lock(mutex_database);

        logToFile("[Database] Loading database at %s\n", DB_FILENAME);

        if(!prepare()) return Passwords{};

        Passwords passwords;
        
        bool opened = R_SUCCEEDED(fsFsOpenFile(&sdmc, DB_FILENAME, FsOpenMode_Read, &handle_database));        

        if(!opened) {
            logToFile("Could not open database file. Try to create a new file.\n");
            
            // Verify whether data directory exists
            FsDirEntryType type;
            if(R_FAILED(fsFsGetEntryType(&sdmc, DATA_DIR, &type))) {
                // The directory does not exist
                if(!createDataDirectory()) {
                    logToFile("[Database] The data directory could not be created.\n");
                    return Passwords{};
                }
            }

            if(R_FAILED(fsFsCreateFile(&sdmc, DB_FILENAME, 0, 0))) {
                logToFile("[Database] Could not create a new database file.\n");
                return Passwords{};
            }
        } else {
            s64 fileSize = 0;
            
            if(R_FAILED(fsFileGetSize(&handle_database, &fileSize))) {
                logToFile("[Database] Could not get database file size\n");
                fsFileClose(&handle_database);
                return Passwords{};
            } else {
                logToFile("[Database] Database file size is %i\n", fileSize);
            }

            if(fileSize == 0) {
                logToFile("[Database] Database file is empty\n");
                fsFileClose(&handle_database);
                return Passwords{};
            }

            u8* data_sessions = new u8[fileSize+1];            
            if(data_sessions == nullptr) {
                logToFile("[Database] Could not create a buffer for sessions\n");
                
                fsFileClose(&handle_database);
                return Passwords{};
            } else {
                logToFile("[Database] sessions buffer ready at @%p\n", (void*)data_sessions);
            }
            
            u64 dataRead = 0;
            if(R_FAILED(fsFileRead(&handle_database, 0, data_sessions, fileSize, FsReadOption_None, &dataRead))) {
                logToFile("[Database] Could not read the database file\n");

                fsFileClose(&handle_database);
                return Passwords{};
            } else {
                logToFile("[Database] Sessions database read\n");
            }

            data_sessions[fileSize] = '\0';
            json j_settings = json::parse(data_sessions);

            std::vector<json> j_entries = j_settings["passwords"].get<std::vector<json>>();

            for(const auto& j_entry: j_entries) {
                auto uidAsString = j_entry["uid"].get<std::string>();
                auto password = j_entry["password"].get<std::string>();
                //auto uid = accountUidFromString(uidAsString);

                passwords[uidAsString] = password;
            }

            fsFileClose(&handle_database);
            delete[] data_sessions;
        }

        return passwords;
    }

    void savePassword(AccountUid account, Password password)
    {
        const auto& passwords = loadPasswords();
        
        std::lock_guard<std::mutex> lock(mutex_database);

        if(!prepare()) return;

        json j_entries;
        for(const auto& [uid, password]: passwords) {
            json j_entry = json::object( {
                { "uid", uid },                
                { "password", password }
            });

            j_entries.push_back(j_entry);
        }

        json j_history = json{
            { "passwords", j_entries }
        };
        
        if(R_FAILED(fsFsDeleteFile(&sdmc, DB_FILENAME))) {
            logToFile("[Database] Could not delete the current database file\n");            
        }

        if(R_FAILED(fsFsCreateFile(&sdmc, DB_FILENAME, 0, 0))) {
            logToFile("[Database] Could not create the database file\n");
            return;
        } else {
            logToFile("[Database] New database file created\n");
        }

        if(R_FAILED(fsFsOpenFile(&sdmc, DB_FILENAME, FsOpenMode_Write | FsOpenMode_Append, &handle_database))) {
            logToFile("[Database] The database file could not be opened for writing\n");
            return;
        }

        const auto data = j_history.dump();
        const auto s_data = data.c_str();

        logToFile("[Database] Writing sessions data %s (size=%i)\n", s_data, std::strlen(s_data));
        if(R_FAILED(fsFileWrite(&handle_database, 0, s_data, std::strlen(s_data), FsWriteOption_Flush))) {
            logToFile("[Database] Could not write into database file\n");
        }

        fsFileClose(&handle_database);
    }

}