#pragma once

#include <string>
#include <memory>
#include <dpp/dpp.h>

class IDiscordManager {
private:
    std::unique_ptr<dpp::cluster> m_bot;

public:
    void Init();
};
