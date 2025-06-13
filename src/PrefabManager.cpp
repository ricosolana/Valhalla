#include "PrefabManager.h"

#include "ZDOManager.h"
#include "VUtilsResource.h"

auto PREFAB_MANAGER = std::make_unique<IPrefabManager>();
IPrefabManager* PrefabManager() {
    return PREFAB_MANAGER.get();
}



const Prefab& Prefab::Instance::GetPrefab() const {
    return PrefabManager()->get_prefab(m_prefabHash);
}



bool Prefab::IsDistant() const noexcept {
    return AllFlagsPresent(Flag::DISTANT);
}

bool Prefab::IsPersistent() const noexcept {
    return AllFlagsPresent(Flag::PERSISTENT);
}

avledet::util::ObjectType Prefab::GetObjectType() const noexcept {
    unsigned int v = (1 & AllFlagsPresent(Flag::TYPE1)) |
        ((1 & AllFlagsPresent(Flag::TYPE2)) << 1);

    assert(v <= 0b11);

    return (avledet::util::ObjectType)v;
}



void IPrefabManager::Init() {
    LOG_INFO(VH_LOGGER, "Initializing PrefabManager");

    auto opt = VUtils::Resource::ReadFile<avledet::util::Bytes>("prefabs.pkg");

    if (!opt) {
        throw std::runtime_error("prefabs.pkg missing");
    }

    DataReader pkg(opt.value());

    pkg.read<std::string_view>(); // comment
    auto ver = pkg.read<std::string_view>();
    if (ver != VConstants::GAME) {
        LOG_WARNING(VH_LOGGER, "prefabs.pkg uses different game version than server ({})", ver);
    }

    auto count = pkg.read<std::int32_t>();

    for (int i=0; i < count; i++) {
        Register(pkg);
    }

    LOG_INFO(VH_LOGGER, "Loaded {} prefabs", count);
}