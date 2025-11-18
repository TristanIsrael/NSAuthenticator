#pragma once

#include <map>
#include <switch.h>
#include "helpers.h"

namespace alefbet::authenticator::database {

    using Password = std::string;
    using Passwords = std::map<UserUid, Password>;

    bool createDataDirectory();

    /* Data management */    
    Passwords loadPasswords();
    void savePassword(AccountUid account, Password password);
}