#include "CompileSettings.h"
#include <sol/property.hpp>

#if AVL_IS_ON(AVL_ENABLE_SCRIPTING)
    #include <sol/forward.hpp>

    #include "ModManager.h"
    #include "Peer.h"


using namespace avledet::util;

void IScriptManager::load_userdata_peer()
{
    // clang-format off

    LOG_DEBUG(AVL_LOGGER, "Initializing API types - peer");

    m_state.new_enum("ChatMsgType", 
        "WHISPER", ChatMsgType::Whisper, 
        "NORMAL", ChatMsgType::Normal, 
        "SHOUT", ChatMsgType::Shout, 
        "PING", ChatMsgType::Ping
    );

    //state.new_usertype<NetRpc>("RpcClient",
    //    "socket", sol::readonly(&Peer::m_socket)
    //    //"Register", sol::
    //
    //    );

    this->new_usertype<Peer>("Peer",
        sol::no_constructor,
        // member fields
        "marker", sol::property(&Peer::IsMapVisible, &Peer::SetMapVisible), //TODO rename this...
        "admin", sol::property(&Peer::IsAdmin, &Peer::SetAdmin), 
        "character_id", sol::property([](Peer &self) -> ZDOID { return self.m_characterID; }),// returns a copy
        "name", sol::readonly(&Peer::m_name),// strings are immutable in Lua similarly to Java
        "pos", sol::readonly(&Peer::m_pos),
        //"uuid", sol::property([](Peer& self) { return Int64Wrapper(self.m_uuid); }),
        "socket", sol::readonly(&Peer::m_socket), 
        "zdo", sol::property(&Peer::find_zdo),
        // member functions
        "kick", &Peer::Kick,
        "chat_message", sol::resolve<void (std::string_view)>(&Peer::ChatMessage),
        "console_message", sol::resolve<void (std::string_view)>(&Peer::ConsoleMessage),
        "corner_message", sol::resolve<void (std::string_view)>(&Peer::CornerMessage),
        "center_message", sol::resolve<void (std::string_view)>(&Peer::CenterMessage),
        "teleport", sol::overload(
            sol::resolve<void(Vector3f pos, Quaternion rot, bool animation)>(&Peer::Teleport),
            sol::resolve<void(Vector3f pos)>(&Peer::Teleport)),
        "disconnect", &Peer::Disconnect, 
        "invoke_self", sol::overload(
            sol::resolve<void(Hash, DataReader &)>(&Peer::InternalInvoke),
            sol::resolve<void(std::string_view, DataReader &)>(&Peer::InternalInvoke)),
        "register", &Peer::RegisterLua,
        "invoke", &Peer::InvokeLua, 
        "route_view", &Peer::RouteViewLua, 
        "route", &Peer::RouteLua
    );

    // clang-format on
}

#endif
