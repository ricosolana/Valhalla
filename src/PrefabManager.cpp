#include "PrefabManager.h"
#include "ZDOManager.h"
#include "VUtilsResource.h"

auto PREFAB_MANAGER(std::make_unique<IPrefabManager>());
IPrefabManager* PrefabManager() {
    return PREFAB_MANAGER.get();
}



const Prefab* IPrefabManager::GetPrefab(HASH_t hash) {
    auto&& find = m_prefabs.find(hash);
    if (find != m_prefabs.end())
        return find->second.get();
    return nullptr;
}

void IPrefabManager::Init() {
    LOG(INFO) << "Initializing PrefabManager";

    auto opt = VUtils::Resource::ReadFile<BYTES_t>("prefabs.pkg");

    if (!opt) {
        throw std::runtime_error("prefabs.pkg missing");
    }

    DataReader pkg(opt.value());

    pkg.Read<std::string>(); // comment
    std::string ver = pkg.Read<std::string>();
    if (ver != VConstants::GAME)
        LOG(WARNING) << "prefabs.pkg uses different game version than server (" << ver << ")";

    auto count = pkg.Read<int32_t>();
    for (int i=0; i < count; i++) {
        auto prefab = std::make_unique<Prefab>();
        prefab->m_name = pkg.Read<std::string>();
        prefab->m_hash = VUtils::String::GetStableHashCode(prefab->m_name);
        prefab->m_type = (ZDO::ObjectType)pkg.Read<int32_t>();

        prefab->m_localScale = pkg.Read<Vector3>();

        prefab->m_flags = pkg.Read<Prefab::Flag>();

        VLOG(1) << "'" << prefab->m_name << "', '" << prefab->m_hash << "'";

        //prefab->m_distant = pkg.Read<bool>();
        //prefab->m_persistent = pkg.Read<bool>();
        //prefab->m_type = (ZDO::ObjectType)pkg.Read<int32_t>();
        //if (pkg.Read<bool>()) // sync initial scale
        //    prefab->m_localScale = pkg.Read<Vector3>();
        //else {
        //    prefab->m_localScale = Vector3(1, 1, 1);
        //}

        m_prefabs.insert({
            VUtils::String::GetStableHashCode(prefab->m_name),
            std::move(prefab) });
    }

    LOG(INFO) << "Loaded " << count << " prefabs";
}

/*
ZDO* IPrefabManager::Instantiate(HASH_t hash, const Vector3& pos, const Quaternion& rot, const Prefab** outPrefab) {
    auto&& find = m_prefabs.find(hash);

    if (find != m_prefabs.end()) {
        auto&& prefab = find->second;

        auto&& zdo = Instantiate(*prefab.get(), pos, rot);

        if (outPrefab) *outPrefab = prefab.get();

        return &zdo;
    }

    return nullptr;
}

ZDO& IPrefabManager::Instantiate(const Prefab& prefab, const Vector3& pos, const Quaternion& rot) {    
    auto &&zdo = ZDOManager()->Instantiate(pos);
    zdo.m_rotation = rot;
    zdo.m_prefab = &prefab;

    if (prefab.FlagsPresent(Prefab::Flag::SyncInitialScale))
        zdo.Set("scale", prefab.m_localScale);

    return zdo;
}

ZDO& IPrefabManager::Instantiate(const ZDO& zdo) {
    auto&& copy = Instantiate(*zdo.m_prefab, zdo.m_pos, zdo.m_rotation);

    const ZDOID temp = copy.m_id; // Copying copies everything (including UID, which MUST be unique for every ZDO)
    copy = zdo;
    copy.m_id = temp;

    return copy;
}*/
