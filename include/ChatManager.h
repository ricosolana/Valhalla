#pragma once

#include "Utils.h"

namespace ChatManager {

    enum class Type : int32_t {
        Whisper,
        Normal,
        Shout,
        Ping
    };

    void Init();

}
