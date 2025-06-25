#include "PrefabManager.h"

#include "VUtilsResource.h"
#include "ZDOManager.h"

auto PREFAB_MANAGER = std::make_unique<IPrefabManager>();

IPrefabManager *PrefabManager()
{
    return PREFAB_MANAGER.get();
}

void IPrefabManager::Init()
{
    LOG_NOTICE(AVL_LOGGER, "Initializing PrefabManager");

    auto opt = VUtils::Resource::ReadFile<avledet::util::Bytes>("prefabs.pkg");

    if (!opt) {
        throw std::runtime_error("prefabs.pkg missing");
    }

    DataReader pkg(opt.value());

    auto comment = pkg.read<std::string_view>();// comment
    LOG_DEBUG(AVL_LOGGER, "pkg comment: {}", comment);

    auto ver = pkg.read<std::string_view>();
    if (ver != VConstants::GAME) {
        LOG_WARNING(AVL_LOGGER, "prefabs.pkg uses different game version than server ({})", ver);
    }

    auto count = pkg.read<std::int32_t>();

    for (int i = 0; i < count; i++) {
        Register(pkg);
    }

    LOG_NOTICE(AVL_LOGGER, "Loaded {}/{} prefabs", m_prefabs.size(), count);
}

Prefab const *IPrefabManager::find_prefab(avledet::util::Hash hash) const
{
    auto &&find = m_prefabs.find(hash);
    if (find != m_prefabs.end())
        return &(*find);
    return nullptr;
}

// Get a prefab by name
//	Returns the prefab or null
Prefab const *IPrefabManager::find_prefab(std::string_view name) const
{
    return find_prefab(avledet::util::get_stable_hash(name));
}

// Get a definite prefab
//	Throws if prefab not found
Prefab const &IPrefabManager::get_prefab(avledet::util::Hash hash) const
{
    auto prefab = find_prefab(hash);
    if (!prefab)
        throw std::runtime_error("prefab not found");
    return *prefab;
}

// Get a definite prefab
//	Throws if prefab not found
Prefab const &IPrefabManager::get_prefab(std::string_view name) const
{
    return get_prefab(avledet::util::get_stable_hash(name));
}

void IPrefabManager::Register(std::string name, Vector3f scale, Prefab::Flag flags)
{
    avledet::util::Hash hash = avledet::util::get_stable_hash(name);
    auto &&emp               = m_prefabs.emplace(Prefab(std::move(name), scale, flags));
    auto &&prefab            = *emp.first;

    assert(!prefab.m_name.empty());

    if (!emp.second) {
        LOG_WARNING(AVL_LOGGER, "Duplicate prefab tried to register: {}", prefab.m_name);
    }

    if (name == "_TerrainCompiler") {
        assert(prefab.GetObjectType() == avledet::util::ObjectType::TERRAIN);
    }
}

void IPrefabManager::Register(DataReader &reader)
{
    auto name       = reader.read<std::string>();
    auto localScale = reader.read<Vector3f>();
    auto flags      = reader.read<Prefab::Flag>();

    Register(std::move(name), localScale, flags);
}
