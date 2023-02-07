#include "ModManager.h"
#include <easylogging++.h>
#include <robin_hood.h>

#include "VUtilsResource.h"
#include "VUtilsString.h"

void IModManager::Init() {

    // load all mods from file

    auto ptr = std::make_unique<Mod>("my mod", sol::environment(m_state, sol::create));
    auto&& env = ptr->m_env;

    auto mod = ptr.get();

    env["print"] = [this, mod](sol::variadic_args args) {
        auto&& tostring(m_state["tostring"]);

        std::string s;
        int idx = 0;
        for (auto&& arg : args) {
            if (idx++ > 0)
                s += " ";
            s += tostring(arg);
        }

        LOG(INFO) << "[" << mod->m_name << "] " << s;
    };

    auto utilsTable = env["VUtils"].get_or_create<sol::table>();

    utilsTable["Compress"] = sol::overload(
        sol::resolve<std::optional<BYTES_t>(const BYTES_t&)>(VUtils::CompressGz),
        sol::resolve<std::optional<BYTES_t>(const BYTES_t&, int)>(VUtils::CompressGz)
    );
    utilsTable["Decompress"] = sol::resolve<std::optional<BYTES_t>(const BYTES_t&)>(VUtils::Decompress);

    utilsTable["Decompress"] = sol::resolve<std::optional<BYTES_t>(const BYTES_t&)>(VUtils::Decompress);
    {
        auto stringUtilsTable = utilsTable["String"].get_or_create<sol::table>();

        stringUtilsTable["GetStableHashCode"] = sol::resolve<HASH_t(const std::string&)>(VUtils::String::GetStableHashCode);
    }
    {
        auto resourceUtilsTable = utilsTable["Resource"].get_or_create<sol::table>();

        /*
        resourceUtilsTable["ReadFileBytes"] = [](const std::string& path) { return VUtils::Resource::ReadFileBytes(path); };
        resourceUtilsTable["ReadFileString"] = [](const std::string& path) { return VUtils::Resource::ReadFileString(path); };
        resourceUtilsTable["ReadFileLines"] = [](const std::string& path) { return VUtils::Resource::ReadFileLines(path); };

        resourceUtilsTable["WriteFileBytes"] = [](const std::string& path, const BYTES_t& bytes) { return VUtils::Resource::WriteFileBytes(path, bytes); };
        resourceUtilsTable["WriteFileString"] = [](const std::string& path, const std::string& s) { return VUtils::Resource::WriteFileString(path, s); };
        resourceUtilsTable["WriteFileLines"] = [](const std::string& path, const std::vector<std::string>& lines) { return VUtils::Resource::WriteFileLines(path, lines); };
        */

        
        /*
        resourceUtilsTable["ReadFileBytes"] = VUtils::Resource::ReadFileBytes;
        resourceUtilsTable["ReadFileString"] = VUtils::Resource::ReadFileString;
        resourceUtilsTable["ReadFileLines"] = VUtils::Resource::ReadFileLines;
        resourceUtilsTable["WriteFileBytes"] = VUtils::Resource::WriteFileBytes;
        resourceUtilsTable["WriteFileString"] = VUtils::Resource::WriteFileString;
        resourceUtilsTable["WriteFileLines"] = VUtils::Resource::WriteFileLines;*/
        
        resourceUtilsTable["ReadFileBytes"] = VUtils::Resource::ReadFileBytes;
        resourceUtilsTable["ReadFileString"] = VUtils::Resource::ReadFileString;
        resourceUtilsTable["ReadFileLines"] = sol::resolve<std::optional<std::vector<std::string>>(const fs::path&)>(VUtils::Resource::ReadFileLines);
        resourceUtilsTable["WriteFileBytes"] = sol::resolve<bool(const fs::path&, const BYTES_t&)>(VUtils::Resource::WriteFileBytes);
        resourceUtilsTable["WriteFileString"] = VUtils::Resource::WriteFileString;
        //resourceUtilsTable["WriteFileLines"] = sol::overload(
        //    sol::resolve<bool(const fs::path&, const std::vector<std::string>&)>(VUtils::Resource::WriteFileLines),
        //    sol::resolve<bool(const fs::path&, const robin_hood::unordered_set<std::string>&)>(VUtils::Resource::WriteFileLines)
        //);
        resourceUtilsTable["WriteFileString"] = sol::resolve<bool(const fs::path&, const std::vector<std::string>&)>(VUtils::Resource::WriteFileLines);

        /*
        resourceUtilsTable["ReadFileBytes"] = sol::resolve<std::optional<BYTES_t>(const std::string&)>(VUtils::Resource::ReadFileBytes);
        resourceUtilsTable["ReadFileString"] = sol::resolve<std::optional<std::string>(const std::string&)>(VUtils::Resource::ReadFileString);
        resourceUtilsTable["ReadFileLines"] = sol::resolve<std::optional<std::vector<std::string>>(const std::string&)>(VUtils::Resource::ReadFileLines);
        resourceUtilsTable["WriteFileBytes"] = sol::resolve<bool(const std::string&, const BYTES_t&)>(VUtils::Resource::WriteFileBytes);
        resourceUtilsTable["WriteFileString"] = sol::resolve<bool(const std::string&, const std::string&)>(VUtils::Resource::WriteFileString);
        resourceUtilsTable["WriteFileLines"] = sol::resolve<bool(const std::string&, const std::vector<std::string>&)>(VUtils::Resource::WriteFileLines);
        */
    }

}

void IModManager::Uninit() {

}
