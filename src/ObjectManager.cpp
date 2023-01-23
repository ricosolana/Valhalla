#include "ObjectManager.h"

#include "ZDOManager.h"
#include "VUtilsResource.h"

void IObjectManager::Init() {
    // load valheim asset files

    // the data below is pretty much all thats needed

    // ZoneLocations will be completely different however
}

ZDO* IObjectManager::Instantiate(HASH_t hash, const Vector3& pos, const Quaternion& rot) {
    auto&& find = m_prefabs.find(hash);

    if (find != m_prefabs.end()) {
        auto&& prefab = find->second;

        auto zdo = ZDOManager()->CreateZDO(pos);
        zdo->m_distant = prefab->m_distant;
        zdo->m_persistent = prefab->m_persistent;
        zdo->m_type = prefab->m_type;
        zdo->m_rotation = rot;
        zdo->m_prefab = hash;

        if (prefab->m_localScale != Vector3(1, 1, 1)) {
            zdo->Set("scale", prefab->m_localScale);
        }

        return zdo;
    }

    return nullptr;
}