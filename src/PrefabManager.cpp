#include "PrefabManager.h"

#include "ZDOManager.h"
#include "VUtilsResource.h"

auto PREFAB_MANAGER(std::make_unique<IPrefabManager>());
IPrefabManager* PrefabManager() {
    return PREFAB_MANAGER.get();
}



//const Prefab Prefab::NONE = Prefab();

void IPrefabManager::Init() {
    LOG_INFO(LOGGER, "Initializing PrefabManager");

    auto opt = VUtils::Resource::ReadFile<BYTES_t>("prefabs.pkg");

    if (!opt) {
        throw std::runtime_error("prefabs.pkg missing");
    }

    DataReader pkg(opt.value());

    pkg.Read<std::string_view>(); // comment
    auto ver = pkg.Read<std::string_view>();
    if (ver != VConstants::GAME)
        LOG_WARNING(LOGGER, "prefabs.pkg uses different game version than server ({})", ver);

    auto count = pkg.Read<int32_t>();
#if VH_IS_OFF(VH_PREFAB_INFO)
    m_prefabs.reserve(count);
#endif

    for (int i=0; i < count; i++) {
        Register(pkg);

        /*
        auto name = pkg.Read<std::string_view>();
        //auto hash = VUtils::String::GetStableHashCode(name);
        auto type = (ObjectType) pkg.Read<int32_t>();
        auto localScale = pkg.Read<Vector3f>();
        auto flags = pkg.Read<uint64_t>();



#if VH_IS_ON(VH_PREFAB_INFO)
        auto&& insert = m_prefabs.insert(Prefab(name, type, localScale, (Prefabs::Flag)flags));
#else
        auto&& insert = m_prefabs.insert(m_prefabs.end(), VUtils::String::GetStableHashCode(name));
#endif*/
    }
    
#if VH_IS_OFF(VH_PREFAB_INFO)
    std::sort(m_prefabs.begin(), m_prefabs.end());
#endif

    LOG_INFO(LOGGER, "Loaded {} prefabs", count);
}