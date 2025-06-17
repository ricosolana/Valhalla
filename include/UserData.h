#pragma once

#include "DataStream.h"
#include <stdexcept>

class UserProfile
{
  public:
    // all relatively confusing verbose names

    std::string m_name;
    std::string m_gamerTag;
    std::string m_networkUserId;

    UserProfile(std::string name, std::string gamerTag, std::string networkUserId) :
        m_name(std::move(name)),
        m_gamerTag(std::move(gamerTag)),
        m_networkUserId(std::move(networkUserId))
    {
    }
};

template<>
struct avledet::util::Streamer<UserProfile>
{
    void operator()(Writer &writer, UserProfile const &value) const
    {
        throw std::runtime_error("nyi");
    }

    decltype(auto) operator()(Reader &reader) const
    {
        throw std::runtime_error("nyi");
        // TODO read seperately, not like UserData(read(), read(), read())
        // because these could be out of order, causing UBF
    }
};
