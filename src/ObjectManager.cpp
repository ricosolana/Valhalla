#include "ObjectManager.h"

#include "ZDOManager.h"

ZDO* IObjectManager::Instantiate(HASH_t hash, const Vector3& pos, const Quaternion& rot) {
    auto&& find = m_prefabs.find(hash);

    if (find != m_prefabs.end()) {

        ZDOManager()->

    }
}