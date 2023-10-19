#include "PrefabManager.h"

#include "ZDOManager.h"
#include "VUtilsResource.h"

auto PREFAB_MANAGER(std::make_unique<IPrefabManager>());
IPrefabManager* PrefabManager() {
    return PREFAB_MANAGER.get();
}



const Prefab& Prefab::Instance::GetPrefab() const {
    return PrefabManager()->RequirePrefabByHash(m_prefabHash);
}

ObjectType Prefab::GetObjectType() const noexcept {
    unsigned int v = (1 & AllFlagsPresent(Flag::TYPE1)) |
        ((1 & AllFlagsPresent(Flag::TYPE2)) << 1);

    assert(v <= 0b11);

    return (ObjectType)v;
}



void IPrefabManager::init() {
    LOG_INFO(LOGGER, "Initializing PrefabManager");

    auto opt = VUtils::Resource::ReadFile<BYTES_t>("prefabs.pkg");

    if (!opt) {
        throw std::runtime_error("prefabs.pkg missing");
    }

    DataReader pkg(opt.value());

    pkg.read<std::string_view>(); // comment
    auto ver = pkg.read<std::string_view>();
    if (ver != VConstants::GAME)
        LOG_WARNING(LOGGER, "prefabs.pkg uses different game version than server ({})", ver);

    auto count = pkg.read<int32_t>();

    for (int i=0; i < count; i++) {
        Register(pkg);
    }

    LOG_INFO(LOGGER, "Loaded {} prefabs", count);
}