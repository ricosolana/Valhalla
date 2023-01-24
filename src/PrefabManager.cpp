#include "PrefabManager.h"
#include "ZDOManager.h"
#include "VUtilsResource.h"
#include "NetPackage.h"

auto PREFAB_MANAGER(std::make_unique<IPrefabManager>());
IPrefabManager* PrefabManager() {
    return PREFAB_MANAGER.get();
}

void IPrefabManager::Init() {
    // load valheim asset files

    // the data below is pretty much all thats needed

    // ZoneLocations will be completely different however

    auto opt = VUtils::Resource::ReadFileBytes("prefabs.pkg");

    if (!opt) {
        throw std::runtime_error("prefabs.pkg missing");
    }

    NetPackage pkg(opt.value());
    auto count = pkg.Read<int32_t>();
    while (count--) {
        auto prefab = std::make_unique<Prefab>();
        prefab->m_name = pkg.Read<std::string>();
        prefab->m_distant = pkg.Read<bool>();
        prefab->m_persistent = pkg.Read<bool>();
        prefab->m_type = (ZDO::ObjectType)pkg.Read<int32_t>();
        if (pkg.Read<bool>())
            prefab->m_localScale = pkg.Read<Vector3>();

        m_prefabs.insert({
            VUtils::String::GetStableHashCode(prefab->m_name),
            std::move(prefab) });
    }
}

ZDO* IPrefabManager::Instantiate(HASH_t hash, const Vector3& pos, const Quaternion& rot) {
    auto&& find = m_prefabs.find(hash);

    size_t ss = sizeof(Prefab);

    if (find != m_prefabs.end()) {
        auto&& prefab = find->second;

        auto zdo = ZDOManager()->CreateZDO(pos);
        zdo->m_distant = prefab->m_distant;
        zdo->m_persistent = prefab->m_persistent;
        zdo->m_type = prefab->m_type;
        zdo->m_rotation = rot;
        zdo->m_prefab = hash;

        if (prefab->m_localScale != Vector3(0, 0, 0)) {
            zdo->Set("scale", prefab->m_localScale);
        }

        return zdo;
    }

    return nullptr;
}