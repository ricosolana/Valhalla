#include "Prefab.h"
#include "PrefabManager.h"

Prefab::Reference Prefab::Instance::get_prefab() const
{
    return PrefabManager::instance().get_prefab(m_prefabHash);
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

bool Prefab::AllFlagsPresent(Flag prefabFlags) const noexcept
{
    return prefabFlags == Flag::NONE
           || (std::to_underlying(m_flags) & std::to_underlying(prefabFlags))
                      == std::to_underlying(prefabFlags);
}

bool Prefab::AnyFlagsPresent(Flag prefabFlags) const noexcept
{
    return prefabFlags == Flag::NONE
           || (std::to_underlying(m_flags) & std::to_underlying(prefabFlags))
                      != std::to_underlying(Flag::NONE);
}

bool Prefab::AllFlagsAbsent(Flag prefabFlags) const noexcept
{
    return prefabFlags == Flag::NONE
           || (std::to_underlying(m_flags) & std::to_underlying(prefabFlags))
                      == std::to_underlying(Flag::NONE);
}

bool Prefab::AnyFlagsAbsent(Flag prefabFlags) const noexcept
{
    return prefabFlags == Flag::NONE
           || (std::to_underlying(m_flags) & std::to_underlying(prefabFlags))
                      != std::to_underlying(prefabFlags);
}

bool Prefab::is_distant() const noexcept
{
    return AllFlagsPresent(Flag::DISTANT);
}

bool Prefab::is_persistent() const noexcept
{
    return AllFlagsPresent(Flag::PERSISTENT);
}

avledet::util::ObjectType Prefab::GetObjectType() const noexcept
{
    unsigned int v = (1 & AllFlagsPresent(Flag::TYPE1)) | ((1 & AllFlagsPresent(Flag::TYPE2)) << 1);

    assert(v <= 0b11);

    return (avledet::util::ObjectType) v;
}

bool Prefab::operator==(std::unique_ptr<Prefab> const& other) const noexcept
{
    return this->m_hash == other->m_hash;
}

bool Prefab::operator==(Prefab::Reference other) const noexcept
{
    return this->m_hash == other.get().m_hash;
}

bool Prefab::operator==(avledet::util::Hash other) const noexcept
{
    return this->m_hash == other;
}

bool Prefab::operator==(std::string_view other) const noexcept
{
    return this->m_hash == avledet::util::get_stable_hash(other);
}

bool operator==(std::unique_ptr<Prefab> const& lhs, std::unique_ptr<Prefab> const& rhs) {
    assert(lhs && rhs);
    return lhs->m_hash == rhs->m_hash;
}

bool operator==(avledet::util::Hash lhs, std::unique_ptr<Prefab> const& rhs) {
    assert(rhs);
    return lhs == rhs->m_hash;
}
