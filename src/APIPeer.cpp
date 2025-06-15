#include "ModManager.h"
#include "Peer.h"

#if VH_IS_ON(VH_USE_MODS)

using namespace avledet::util;

void avledet::api::init_peer(sol::table table) {
    table.new_usertype<IModManager::MethodSig>("MethodSig",
        sol::constructors<IModManager::MethodSig(std::string_view, sol::variadic_args)>()
    );

    table.new_enum("ChatMsgType",
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

    table.new_usertype<Peer>("Peer",
        // member fields
        //"visibleOnMap", &Peer::m_visibleOnMap,
        "marker", sol::property(&Peer::IsMapVisible, &Peer::SetMapVisible),
        //"admin", &Peer::m_admin,
        "admin", sol::property(&Peer::IsAdmin, &Peer::SetAdmin),
        "character_id", sol::property([](Peer& self) -> ZDOID { return self.m_characterID; }), // return copy
        "name", sol::readonly(&Peer::m_name), // strings are immutable in Lua similarly to Java
        "pos", &Peer::m_pos,
        //"uuid", sol::property([](Peer& self) { return Int64Wrapper(self.m_uuid); }),
        "socket", sol::readonly(&Peer::m_socket),
        "zdo", sol::property(&Peer::GetZDO),
        // member functions
        "kick", sol::resolve<void ()>(&Peer::Kick),
        // message functions
        "chat_message", static_cast<void (Peer::*)(std::string_view)>(&Peer::ChatMessage),
        "console_message", static_cast<void (Peer::*)(std::string_view)>(&Peer::ConsoleMessage),
        //"ConsoleMessage", sol::resolve<void(std::string_view)>(&Peer::ConsoleMessage),
        //"ConsoleMessage", &Peer::ConsoleMessage,
        "corner_message", static_cast<void (Peer::*)(std::string_view)>(&Peer::CornerMessage),
        "center_message", static_cast<void (Peer::*)(std::string_view)>(&Peer::CenterMessage),
        // misc functions
        "teleport", sol::overload(
            sol::resolve<void (Vector3f pos, Quaternion rot, bool animation)>(&Peer::Teleport),
            sol::resolve<void (Vector3f pos)>(&Peer::Teleport)
        ),
        //"MoveTo", sol::overload(
        //    sol::resolve<void(const Vector3f& pos, const Quaternion& rot)>(&Peer::MoveTo),
        //    sol::resolve<void(const Vector3f& pos)>(&Peer::MoveTo)
        //),
        "disconnect", &Peer::Disconnect,
        "invoke_self", sol::overload(
            sol::resolve<bool (Hash, DataReader&)>(&Peer::InternalInvoke),
            sol::resolve<bool (std::string_view, DataReader&)>(&Peer::InternalInvoke)
        ),

            //static_cast<void (Peer::*)(const std::string&, DataReader)>(&Peer::InvokeSelf), //  &Peer::InvokeSelf,
            //static_cast<void (Peer::*)(Hash, DataReader)>(&Peer::InvokeSelf)), //  &Peer::InvokeSelf,
        //"Register", [](Peer& self, const MethodSig &repr, sol::function func) {
        //    self.Register(repr.m_hash, func, repr.m_types);
        //},

        // static_cast<void (DataWriter::*)(const Bytes&, std::size_t)>(&DataWriter::write),
        "register", &Peer::RegisterLua,
        //"Register", [](Peer& self, const IModManager::MethodSig& sig, const sol::function& func, sol::this_environment te) { 
        //    sol::environment& env = te;
        //    Mod& mod = env["this"].get<sol::table>().as<Mod&>();
        //    self.RegisterLua(sig, func, &mod); 
        //},
        "invoke", &Peer::InvokeLua,
        "route_view", &Peer::RouteViewLua,
        "route", &Peer::RouteLua
        //sol::overload(
        //    sol::resolve<void(const ZDOID&, const IModManager::MethodSig&, const sol::variadic_args&)>(&Peer::RouteLua),
        //    sol::resolve<void(const IModManager::MethodSig&, const sol::variadic_args&)>(&Peer::RouteLua)
        //),

        //"GetMethod", static_cast<IMethod<Peer*>* (Peer::*)(const std::string&)>(&Peer::GetMethod)
        //"GetMethod", sol::overload(
        //    sol::resolve<IMethod<Peer*>* (Hash)>(&Peer::GetMethod),
        //    sol::resolve<IMethod<Peer*>* (const std::string&)>(&Peer::GetMethod)
        //
        //    //static_cast<IMethod<Peer*>* (Peer::*)(const std::string&)>(&Peer::GetMethod)
        //)
    );
}

#endif
