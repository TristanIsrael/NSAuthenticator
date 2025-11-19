#include "helpers.h"
#include <sstream>
#include <iomanip>
#include <cinttypes>
#include <iostream>
#include <vector>
#include <string_view>
#include <ranges>
#include <codecvt>
#include <switch.h>
#include "logger.h"
#ifdef CAN_REBOOT_TO_PAYLOAD
#include "ams_bpc.h"
#endif

using namespace alefbet::authenticator::logger;
using namespace alefbet::authenticator::structs;
using std::operator""sv;

namespace alefbet::authenticator::helpers {    

    std::string titleIdToString(u64 titleId) {
        if(titleId == 0) {
            return std::string("None");        
        }
        
        std::stringstream ss;
        ss << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << titleId;
        return ss.str();
    }

    UserUid accountUidToString(AccountUid uid) {
        return std::to_string(uid.uid[0]) + ":" + std::to_string(uid.uid[1]);
    }

    AccountUid accountUidFromString(const UserUid& uid_str) {
        AccountUid uid;

        std::vector<std::string> parts;
        for (const auto part : std::views::split(uid_str, ":"sv)) {
            parts.push_back(std::string(part.begin(), part.end()));
        }
        
        if(parts.size() != 2) {
            logToFile("[Helpers] Incorrect split of AccountUid %s.\n", uid_str.c_str());
            return uid;
        }

        uid.uid[0] = std::stoull(parts[0]);
        uid.uid[1] = std::stoull(parts[1]);

        return uid;
    }

    UserData getUserFromAccountUid(AccountUid uid) {
        UserData user;

        ::Result rc = accountInitialize(AccountServiceType_Administrator);
        if(R_FAILED(rc)) {
            logToFile("[Helpers] Could not initialize account service: %i:%i\n", R_MODULE(rc), R_DESCRIPTION(rc));
            user.nickname = UserNickname("ERR#003");
            return user;
        }

        AccountProfile profile;
        AccountUserData user_data;
        AccountProfileBase base;
        rc = accountGetProfile(&profile, uid); 
        if(R_FAILED(rc)) {
            logToFile("[Helpers] Could not get account profile: %i\n", rc);
            accountExit();
            user.nickname = UserNickname("ERR#005");
            return user;
        } else {
            logToFile("[Helpers] accountGetProfile() ok\n");
        }

        rc = accountProfileGet(&profile, &user_data, &base);
        if(R_FAILED(rc)) {
            logToFile("[Helpers] Could not get user data: %i\n", rc);
            accountProfileClose(&profile);
            accountExit();
            user.nickname = UserNickname("ERR#006");
            return user;
        } else {
            logToFile("[Helpers] accountProfileGet() ok\n");
        }
        
        user.uid = uid;
        user.nickname = UserNickname(base.nickname);
        UserUid uidstr = accountUidToString(uid);
        //logToFile("[Helpers] uid=%s, Nickname=%s\n", uidstr.c_str(), user.nickname.c_str());

        accountProfileClose(&profile);
        accountExit();

        return user;
    }

    UserData getCurrentUser() {
        UserData user;

        ::Result rc = accountInitialize(AccountServiceType_Administrator);
        if(R_FAILED(rc)) {
            logToFile("[Helpers] Could not initialize account service: %i:%i\n", R_MODULE(rc), R_DESCRIPTION(rc));
            user.nickname = UserNickname("ERR#003");
            return user;
        }

        AccountUid uid;
        rc = accountGetPreselectedUser(&uid);
        //logToFile("rc=%i, Uid=%i.%i\n", rc, uid.uid[0], uid.uid[1]);
        rc = accountGetLastOpenedUser(&uid);            
        if(R_FAILED(rc)) {
            logToFile("[Helpers] Could not get preselected user: %i:%i\n", R_MODULE(rc), R_DESCRIPTION(rc));
            accountExit();
            user.nickname = UserNickname("ERR#004");
            return user;
        }

        logToFile("[Helpers] user uid: %i.%i\n", uid.uid[0], uid.uid[1]);
        user.uid = uid;

        AccountProfile profile;
        AccountUserData user_data;
        AccountProfileBase base;
        rc = accountGetProfile(&profile, uid); 
        if(R_FAILED(rc)) {
            logToFile("[Helpers] Could not get account profile: %i\n", rc);
            accountExit();
            user.nickname = UserNickname("ERR#005");
            return user;
        } /*else {
            logToFile("[Helpers] accountGetProfile() ok\n");
        }*/

        rc = accountProfileGet(&profile, &user_data, &base);
        if(R_FAILED(rc)) {
            logToFile("[Helpers] Could not get user data: %i\n", rc);
            accountProfileClose(&profile);
            accountExit();
            user.nickname = UserNickname("ERR#006");
            return user;
        } /*else {
            logToFile("[Helpers] accountProfileGet() ok\n");
        }*/
        
        user.nickname = UserNickname(base.nickname);
        logToFile("[Helpers] uid=%i:%i, Nickname=%s\n", user.uid.uid[0], user.uid.uid[1], user.nickname.c_str());

        accountProfileClose(&profile);
        accountExit();

        return user;
    }

    u64 getRunningApplicationPid() {
        u64 process_id = 0;                        

        ::Result rc = pmdmntGetApplicationProcessId(&process_id);
        if(R_FAILED(rc)) {
            //logToFile("[Helpers] Could not get application process ID: %i:%i\n", R_MODULE(rc), R_DESCRIPTION(rc));
            return 0;
        }

        //logToFile("[Helpers] process ID=%i\n", process_id);

        return process_id;
    }

    u64 getRunningApplicationTitleId(u64 process_id) {
        u64 title_id = 0;

        ::Result rc = pmdmntGetProgramId(&title_id, process_id);
        if(R_FAILED(rc)) {
            //logToFile("[Helpers] Could not get the title ID for the process %i:%i\n", R_MODULE(rc), R_DESCRIPTION(rc));
            return 0;
        }
        
        auto formatted_title_id = titleIdToString(title_id);
        //logToFile("[Helpers] title formatted ID=%s, ID=%i, pid=%i\n", formatted_title_id.c_str(), title_id, process_id);

        return title_id;
    }

    std::string getApplicationName(u64 title_id) {
        NsApplicationControlData control;
        u64 actual_size = 0;
        
        ::Result rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, title_id, std::addressof(control), sizeof(control), &actual_size);
        if(R_FAILED(rc)) {
            logToFile("[Helpers] Could not get application control data for title %s\n", titleIdToString(title_id));
            return "Unknown";
        }

        NacpLanguageEntry* langEntry = nullptr;
        rc = nacpGetLanguageEntry(&control.nacp, &langEntry);

        return std::string(langEntry->name);
    }

    std::string today() {        
        ::Result rc = timeInitialize();
        if(R_FAILED(rc)) {
            logToFile("[Helpers] Could not connect to time service\n");
            return "";
        }

        u64 ts;
        TimeCalendarTime time;
        TimeCalendarAdditionalInfo info;
        rc = timeGetCurrentTime(TimeType_LocalSystemClock, &ts);        
        if(R_FAILED(rc)) {
            logToFile("[Helpers] Could not get current time (err %i:%i)\n", R_MODULE(rc), R_DESCRIPTION(rc));
            return "";
        }

        rc = timeToCalendarTimeWithMyRule(ts, &time, &info);
        if(R_FAILED(rc)) {
            logToFile("[Helpers] Could not convert timestamp\n");
            return "";
        }
        
        char buffer[13] = {0};
        std::snprintf(buffer, sizeof(buffer), "%04d%02d%02d",
            time.year,
            time.month,
            time.day
        );

        return std::string(buffer);
    }

    std::string encodePassword(const std::vector<u64>& password) {
        std::string pin;

        if(password.size() != 4) {
            logToFile("[Helpers] The PIN size is wrong.");
            return pin;
        }

        pin = std::to_string(password[0]) +"," +std::to_string(password[1]) +"," +std::to_string(password[2]) +"," +std::to_string(password[3]);        

        return pin;    
    }

#ifdef CAN_REBOOT_TO_PAYLOAD
    static FsFileSystem sdmc;
    
    bool readPayloadFile(u8* buffer, u64 buffer_size) {
        FsFile handle;
        
        ::Result res = fsOpenSdCardFileSystem(&sdmc);
        if(R_FAILED(res)) {
            logToFile("[Helpers] Could not open SDMC\n");
            return false;
        }

        res = fsFsOpenFile(&sdmc, "/atmosphere/reboot_payload.bin", FsOpenMode_Read, &handle);
        if(R_FAILED(res)) {
            logToFile("[Helpers] Could not open payload file\n");
            return false;
        }

        s64 fileSize = 0;
        res = fsFileGetSize(&handle, &fileSize);
        if(R_FAILED(res)) {
            logToFile("[Helpers] Could not get file size\n");
            fsFileClose(&handle);
            return false;
        }

        logToFile("[Helpers] Payload file size is %i bytes\n", fileSize);

        u64 dataRead = 0;
        res = fsFileRead(&handle, 0, buffer, buffer_size, FsReadOption_None, &dataRead);
        if(R_FAILED(res)) {
            logToFile("[Helpers] Could not read %i bytes from payload file\n", buffer_size);
            fsFileClose(&handle);
            return false;
        }

        fsFileClose(&handle);

        logToFile("[Helpers] Read %i bytes from payload file\n", dataRead);
        return true;
    }

    #define IRAM_PAYLOAD_MAX_SIZE 0x24000
    //static u8 g_reboot_payload[IRAM_PAYLOAD_MAX_SIZE];
    bool rebootToPayload() {
        logToFile("[Helpers] Try to reboot to payload\n");
      
        u8 *g_reboot_payload = new u8[IRAM_PAYLOAD_MAX_SIZE];
        smInitialize();
        if(!readPayloadFile(g_reboot_payload, IRAM_PAYLOAD_MAX_SIZE)) {
            logToFile("[Helpers] No payload, shutting down.");
            bpcShutdownSystem();
            return false;
        }

        logToFile("1\n");
        ::Result rc = spsmInitialize();        
        if(R_FAILED(rc)) {
            logToFile("[Helpers] Failed to initialize SPSM\n");
            bpcShutdownSystem();
            return false;
        }
        logToFile("2\n");
        
        smExit(); //Required to connect to ams:bpc       
        logToFile("3\n");

        rc = amsBpcInitialize();
        if (R_FAILED(rc)) {
            logToFile("[Helpers] Failed to initialize ams:bpc: %i\n", rc);
            logToFile("[Helpers] Shutting down.\n");
            bpcShutdownSystem();
            return false;
        }
        logToFile("4\n");        

        logToFile("[Helpers] Calling amsBpcSetRebootPayload\n");        
        if (R_FAILED(amsBpcSetRebootPayload(g_reboot_payload, IRAM_PAYLOAD_MAX_SIZE))) {
            logToFile("[Helpers] Failed to set reboot to payload: %i\n", rc);
            logToFile("[Helpers] Shutting down.\n");

            bpcShutdownSystem();
        } else {
            logToFile("[Helpers] Rebooting to payload\n");

            spsmShutdown(true);
        }

        return true;
    }
#endif

}