#include "PrefabManager.h"

#include "VUtilsResource.h"
#include "ZDOManager.h"

auto PREFAB_MANAGER = std::make_unique<IPrefabManager>();

IPrefabManager *PrefabManager()
{
    return PREFAB_MANAGER.get();
}

Prefab const &Prefab::Instance::GetPrefab() const
{
    return PrefabManager()->get_prefab(m_prefabHash);
}

Prefab::Prefab(std::string name, avledet::util::CSU::Vector3f const &localScale, Flag flags) :
    m_name(std::move(name)),
    m_localScale(localScale),
    m_flags(flags),
    m_hash(avledet::util::get_stable_hash(m_name))//dependent assign, WATCH THE ORDER!
{
    assert(!m_name.empty());
    assert(m_hash);
}

bool Prefab::IsDistant() const noexcept
{
    return AllFlagsPresent(Flag::DISTANT);
}

bool Prefab::IsPersistent() const noexcept
{
    return AllFlagsPresent(Flag::PERSISTENT);
}

avledet::util::ObjectType Prefab::GetObjectType() const noexcept
{
    unsigned int v = (1 & AllFlagsPresent(Flag::TYPE1)) | ((1 & AllFlagsPresent(Flag::TYPE2)) << 1);

    assert(v <= 0b11);

    return (avledet::util::ObjectType) v;
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

    //assert(count == (std::int32_t) m_prefabs.size());

    LOG_NOTICE(AVL_LOGGER, "Loaded {} prefabs", count);
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
    Prefab prefab(name, scale, flags);
    auto &&emp = m_prefabs.emplace(prefab);

    if (!emp.second) {
        LOG_WARNING(AVL_LOGGER, "Duplicate prefab tried to register: {}", name);
    }

    //assert(emp.second);//hmm, why wasnt this inserted?

    assert(!name.empty());

    if (name == "_TerrainCompiler") {
        assert(prefab.GetObjectType() == avledet::util::ObjectType::TERRAIN);
    }

    //VLOG(1) << "'" << prefab.m_name << "', '" << prefab.m_hash << "'";
}

void IPrefabManager::Register(DataReader &reader)
{
    auto name       = reader.read<std::string>();
    auto localScale = reader.read<Vector3f>();
    auto flags      = reader.read<Prefab::Flag>();

    //auto hash = avledet::util::get_stable_hash(name);
    Register(std::move(name), localScale, flags);

    //VLOG(1) << "'" << prefab.m_name << "', '" << prefab.m_hash << "'";
}
