#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtl/btree.hpp>

#include "BitPack.h"
#include "DataStream.h"
#include "Hashes.h"
#include "PrefabManager.h"
#include "Quaternion.h"
#include "Types.h"
#include "ValhallaServer.h"
#include "Vector.h"
#include "VUtils.h"
#include "VUtilsMathf.h"
#include "ZDOConnector.h"
#include "ZDOID.h"
#include "ZoneManager.h"

class ZDO
{
    // TODO are these friend classes safe?
    friend class IZDOManager;
    friend class IPrefabManager;
    friend class VHTest;
    friend class IValhalla;

  public:
    class Rev
    {
      private:
        // DataRevision: 0, OwnerRevision: 1
        //BitPack<std::uint32_t, 21, 32 - 21> m_pack;
        BitPack<std::uint32_t, 23, 32 - 23> m_pack;

        static constexpr auto DATA_REVISION_PACK_INDEX  = 0;
        static constexpr auto OWNER_REVISION_PACK_INDEX = 1;

      public:
        Rev() {}

        Rev(std::uint32_t dataRev, std::uint16_t ownerRev)
        {
            SetDataRevision(dataRev);
            SetOwnerRevision(ownerRev);
        }

        [[nodiscard]] std::uint32_t GetDataRevision() const
        {
            return m_pack.get<DATA_REVISION_PACK_INDEX>();
        }

        [[nodiscard]] std::uint16_t GetOwnerRevision() const
        {
            return m_pack.get<OWNER_REVISION_PACK_INDEX>();
        }

        void SetDataRevision(std::uint32_t dataRev)
        {
            m_pack.set<DATA_REVISION_PACK_INDEX>(dataRev);
        }

        void SetOwnerRevision(std::uint16_t ownerRev)
        {
            m_pack.set<OWNER_REVISION_PACK_INDEX>(ownerRev);
        }

        void ReviseData()
        {
            this->SetDataRevision(GetDataRevision() + 1);
        }

        void ReviseOwner()
        {
            this->SetOwnerRevision(GetOwnerRevision() + 1);
        }
    };

  private:
    static constexpr unsigned int MACHINE_Persistent = 0;
    static constexpr unsigned int MACHINE_Distant    = 1;
    static constexpr unsigned int MACHINE_Type1      = 2;
    static constexpr unsigned int MACHINE_Type2      = 3;

    static constexpr unsigned int NETWORK_Connection = 0;
    static constexpr unsigned int NETWORK_Float      = 1;
    static constexpr unsigned int NETWORK_Vec3       = 2;
    static constexpr unsigned int NETWORK_Quat       = 3;
    static constexpr unsigned int NETWORK_Int        = 4;
    static constexpr unsigned int NETWORK_Long       = 5;
    static constexpr unsigned int NETWORK_String     = 6;
    static constexpr unsigned int NETWORK_ByteArray  = 7;
    static constexpr unsigned int NETWORK_Persistent = 8;
    static constexpr unsigned int NETWORK_Distant    = 9;
    static constexpr unsigned int NETWORK_Type1      = 10;
    static constexpr unsigned int NETWORK_Type2      = 11;
    static constexpr unsigned int NETWORK_Rotation   = 12;


    //using member_hash = std::uint64_t;
    //using member_tuple = std::tuple<float, Vector3f, Quaternion, std::int32_t, std::int64_t, std::string, avledet::util::Bytes>;
    //using member_variant = VUtils::Traits::tuple_to_variant<member_tuple>::type;
    //using member_map = avledet::util::Map<member_hash, member_variant>;

    template<typename T>
    using is_member
            = VUtils::Traits::tuple_has_type<std::remove_cvref_t<T>,
                                             std::tuple<float, Vector3f, Quaternion, std::int32_t,
                                                        std::int64_t, std::string, std::vector<char>>>;

    template<typename T>
    static constexpr bool is_member_v = is_member<T>::value;

    struct hash
    {
        using is_transparent = void;// enable heterogeneous overloads
        using is_avalanching = void;// mark class as high quality avalanching hash

        [[nodiscard]] auto operator()(std::unique_ptr<ZDO> const &value) const noexcept -> std::uint64_t
        {
            assert(value);
            return ankerl::unordered_dense::hash<avledet::util::ZDOID> {}(value->m_id);
        }

        [[nodiscard]] auto operator()(ZDO const *v) const noexcept -> std::uint64_t
        {
            return ankerl::unordered_dense::hash<ZDOID> {}(v->m_id);
        }

        [[nodiscard]] auto operator()(avledet::util::ZDOID const &value) const noexcept -> std::uint64_t
        {
            return ankerl::unordered_dense::hash<avledet::util::ZDOID> {}(value);
        }
    };

    struct equal_to//<std::unique_ptr<ZDO>>
    {
        using is_transparent = void;

        bool operator()(std::unique_ptr<ZDO> const &lhs, std::unique_ptr<ZDO> const &rhs) const
        {
            assert(lhs && rhs);
            return lhs->GetID() == rhs->GetID();
        }

        bool operator()(ZDO *const &lhs, ZDO *const &rhs) const
        {
            assert(lhs && rhs);
            return lhs->GetID() == rhs->GetID();
        }

        bool operator()(ZDOID const &lhs, std::unique_ptr<ZDO> const &rhs) const
        {
            assert(lhs && rhs);
            return lhs == rhs->GetID();
        }

        bool operator()(ZDOID const &lhs, ZDO *const &rhs) const
        {
            assert(lhs && rhs);
            return lhs == rhs->GetID();
        }

        //bool operator()(std::unique_ptr<ZDO> const& rhs, ZDOID const& lhs) const
        //{
        //    assert(lhs && rhs);
        //    return lhs == rhs->GetID();
        //}
    };


    template<class T>
    using Tree = gtl::btree_map<avledet::util::Hash, T>;

    template<class T>
    using VarMap = ankerl::unordered_dense::segmented_map<avledet::util::ZDOID, Tree<T>, ZDO::hash,
                                                          std::equal_to<>>;

    // zdo hash members
    //ankerl::unordered_dense::map<avledet::util::ZDOID, std::pair<ZDO::ConnectionType, avledet::util::Hash>, avledet::sync::ZDO::hash, std::equal_to<>> s_connectionsHashData;

    static inline VarMap<float> m_floats;
    static inline VarMap<avledet::util::CSU::Vector3f> m_vec3;
    static inline VarMap<avledet::util::CSU::Quaternion> m_quats;
    static inline VarMap<std::int32_t> m_ints;
    static inline VarMap<std::int64_t> m_longs;
    static inline VarMap<std::string> m_strings;
    static inline VarMap<std::vector<char>> m_byteArrays;

    static inline ankerl::unordered_dense::segmented_map<ZDOID, ZDOConnectorTargeted>
            ZDO_TARGETED_CONNECTORS;// Current linked connectors
    static inline ankerl::unordered_dense::segmented_map<ZDOID, ZDOConnectorData>
            ZDO_CONNECTORS;         // Saved typed-connectors
    static inline ankerl::unordered_dense::segmented_map<ZDOID, std::int64_t> ZDO_OWNERS;

    template<class T>
        requires is_member_v<T>
    static VarMap<T> &_GetVars()
    {
        if constexpr (std::is_same_v<T, float>) {
            return m_floats;
        } else if constexpr (std::is_same_v<T, avledet::util::CSU::Vector3f>) {
            return m_vec3;
        } else if constexpr (std::is_same_v<T, avledet::util::CSU::Quaternion>) {
            return m_quats;
        } else if constexpr (std::is_same_v<T, std::int32_t>) {
            return m_ints;
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
            return m_longs;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return m_strings;
        } else if constexpr (std::is_same_v<T, std::vector<char>>) {
            return m_byteArrays;
        }
        std::unreachable();
    }

    template<class T>
    static bool _set(Tree<T> &tree, avledet::util::Hash key, T data)
    {
        auto &&entry = tree.try_emplace(key);
        if (entry.second || entry.first->second != data) {// if a modification took place
            entry.first->second = std::move(data);
            return true;
        }

        // else, nothing changed...
        return false;
    }

    //template <class T>
    //static std::pair<bool, Tree<T>*> _GetVarTree(avledet::util::ZDOID const& uid) {
    //    auto&& map = _GetVars<T>();
    //    auto&& emp = map.try_emplace(uid);
    //    return { emp.second, emp.first->second };
    //}

    template<class T, bool create = true>
    static std::pair<bool, Tree<T> *> _GetVarTree(avledet::util::ZDOID const &uid)
    {
        auto &&map = _GetVars<T>();
        if constexpr (create) {
            auto &&emp = map.try_emplace(uid);

            return {emp.second, &emp.first->second};
        } else {
            auto &&itr = map.find(uid);
            if (itr == map.end()) {
                return {false, nullptr};
            } else {
                return {true, &itr->second};
            }
        }
    }

    template<class T>
    //requires is_member_v<T> //std::remove_cvref_t<T>>
    static bool _set(avledet::util::ZDOID const &uid, avledet::util::Hash key, T data)
    {
        auto &&[inserted, tree] = _GetVarTree<T>(uid);
        assert(tree);
        return _set(*tree, key, std::move(data)) || inserted;
    }

    template<class T>
    bool set(avledet::util::Hash key, T data)
    {
        if (_set(m_id, key, std::move(data))) {
            Revise();
            return true;
        }
        return false;
    }

    template<class T>
    bool set(std::string_view key, T data)
    {
        return set(avledet::util::get_stable_hash(key), std::move(data));
    }

    static inline auto strippable_nodes = ankerl::unordered_dense::set<avledet::util::Hash>(
            {avledet::util::get_stable_hash("generated"), avledet::util::get_stable_hash("patrolSpawnPoint"),
             avledet::util::get_stable_hash("autoDespawn"), avledet::util::get_stable_hash("targetHear"),
             avledet::util::get_stable_hash("targetSee"), avledet::util::get_stable_hash("burnt0"),
             avledet::util::get_stable_hash("burnt1"), avledet::util::get_stable_hash("burnt2"),
             avledet::util::get_stable_hash("burnt3"), avledet::util::get_stable_hash("burnt4"),
             avledet::util::get_stable_hash("burnt5"), avledet::util::get_stable_hash("burnt6"),
             avledet::util::get_stable_hash("burnt7"), avledet::util::get_stable_hash("burnt8"),
             avledet::util::get_stable_hash("burnt9"), avledet::util::get_stable_hash("burnt10"),
             avledet::util::get_stable_hash("LookDir"), avledet::util::get_stable_hash("RideSpeed")});

    static inline auto &&strippable_long_nodes = ankerl::unordered_dense::set<avledet::util::Hash>({
            avledet::util::get_stable_hash("user_u"),
            avledet::util::get_stable_hash("user_i"),
            avledet::util::get_stable_hash("RodOwner_u"),
            avledet::util::get_stable_hash("RodOwner_i"),
            avledet::util::get_stable_hash("CatchID_u"),
            avledet::util::get_stable_hash("CatchID_i"),
    });

    static bool can_strip(std::int32_t key)
    {
        return strippable_nodes.contains(key);
    }

    static bool can_strip(std::int32_t key, float data)
    {
        return strippable_nodes.contains(key)
               || (key == avledet::util::get_stable_hash("scaleScalar")
                   && avledet::util::CSU::equal(data, 1.f));
    }

    static bool can_strip(std::int32_t key, avledet::util::CSU::Quaternion const &data)
    {
        return data == avledet::util::CSU::Quaternion::IDENTITY || can_strip(key);
    }

    static bool can_strip(std::int32_t key, std::int32_t data)
    {
        return data == 0 || can_strip(key);
    }

    static bool can_strip(std::int32_t key, std::int64_t data)
    {
        return data == 0 || can_strip(key) || strippable_long_nodes.contains(key);
    }

    static bool can_strip(std::int32_t key, std::string const &data)
    {
        return data.empty() || can_strip(key);
    }

    static bool can_strip(std::int32_t key, std::vector<char> const &data)
    {
        return data.empty() || can_strip(key);
    }

    // (Keep as a member function, to access m_id as needed in future)
    template<class T>
        requires(!std::is_same_v<T, avledet::util::CSU::Vector3f>)
    bool try_convert(std::int32_t key, T const &data)
    {
        return can_strip(key, data);
    }

    // (Keep as a member function, to access m_id as needed in future)
    bool try_convert(std::int32_t key, avledet::util::CSU::Vector3f data)
    {
        if (can_strip(key)) {
            return true;
        }
        if (key == avledet::util::get_stable_hash("SpawnPoint")) {
            //ZDOExtraData.Set(zdoid, ZDOVars.s_spawnPoint, data);
            _set(m_id, avledet::util::get_stable_hash("spawnpoint"), data);
            return true;
        }

        // if x == y == z
        if (avledet::util::CSU::equal(data.x, data.y) && avledet::util::CSU::equal(data.y, data.z)) {
            if (key == avledet::util::get_stable_hash("scale")) {
                // 1 is unit scale
                if (avledet::util::CSU::equal(data.x, 1.f)) {
                    return true;
                }
                _set(m_id, avledet::util::get_stable_hash("scale"), data.x);
                //ZDOExtraData.Set(zid, ZDOVars.s_scaleScalarHash, data.x);
                return true;
            }
            // 0 is default ZDO when no mapping exists
            else if (avledet::util::CSU::equal(data.x, 0.f)) {
                return true;
            }
        }
        return false;
    }

    static std::uint32_t read_num_items(avledet::util::Reader &reader, int version)
    {
        if (version < 33) {
            return (std::uint32_t) reader.read<std::uint8_t>();
        }
        auto num = (std::uint32_t) reader.read<std::uint8_t>();
        if ((num & 128) != 0) {
            num = ((num & 127) << 8) | (std::uint32_t) reader.read<std::uint8_t>();
        }
        return num;
    };

    template<class T>
    void load_vars(avledet::util::Reader &reader, int version, VarMap<T> &map)
    {
        // TODO
        //  these lambdas should be used as ZDO member functions instead (due to some other uses...)


        auto num3     = read_num_items(reader, version);
        auto &&insert = map.try_emplace(m_id);
        //auto&& pair = map.insert({m_id, std::vector<std::pair<int, T>>()});
        // https://stackoverflow.com/a/27553949/9044814
        //auto&& pair = map.try_emplace(std::piecewise_construct,
        //    std::forward_as_tuple(uid),
        //    std::forward_as_tuple());
        auto &&tree = insert.first->second;
        //tree.reserve(num3);
        for (decltype(num3) i = 0; i < num3; i++) {
            int num4  = reader.read<avledet::util::Hash>();
            auto num5 = reader.read<T>();
            if (!try_convert(num4, num5)) {
                //tree.push_back({ num4, num5 });
                tree[num4] = num5;
            }
        }

        if (tree.empty()) {// remove empty vars to save space
            map.erase(insert.first);

            // !!! WARNING !!! do NOT access 'insert'!
        }
    }


  public:
    using reference = ZDO *;
    using optional  = ZDO *;

    // unique: owned + safe
    // soft: indirect through ID
    // set/list...

    using unique_set     = avledet::util::Set<std::unique_ptr<ZDO>, hash, equal_to>;
    using soft_set       = avledet::util::Set<ZDOID, hash, std::equal_to<>>;// hetero hash?
    using reference_set  = avledet::util::Set<reference, hash, equal_to>;
    using reference_list = std::vector<reference>;
    using soft_list      = std::vector<ZDOID>;
    using Filter         = std::function<bool(reference)>;

    [[nodiscard]] static reference make_reference(unique_set::iterator itr)
    {
        return itr->get();
    }

    [[nodiscard]] static optional make_reference(unique_set::value_type const &itr)
    {
        return itr.get();
    }

    [[nodiscard]] static optional make_optional(reference v)
    {
        return v;
    }

    [[nodiscard]] static optional make_optional(unique_set::iterator itr)
    {
        return itr->get();
    }

    [[nodiscard]] static optional make_optional(unique_set::value_type const &itr)
    {
        return itr.get();
    }

    static inline auto const nullopt = nullptr;

  private:
    template<typename T>
    [[maybe_unused]] bool _Set(avledet::util::Hash key, T value)
    {
        //return _Set(key, std::move(value), ZDO_MEMBERS[GetID()]);
        //return ZDOManager()->GetMember()
        return _set(m_id, key, std::move(value));
    }

    static void WriteNumItems(DataWriter &writer, int numItems)
    {
        if (numItems < 128) {
            writer.write((std::uint8_t) numItems);
            return;
        }
        writer.write((std::uint8_t)((numItems >> 8) | 128));
        writer.write((std::uint8_t) numItems);
    }

    template<typename T>
        requires is_member_v<T>
    decltype(auto) _TryWriteType(DataWriter &writer) const
    {//}, Tree<float>& tree) {
        auto &&[_, tree_ptr] = _GetVarTree<T, false>(m_id);
        if (tree_ptr) {
            auto &&tree      = *tree_ptr;
            auto const count = tree.size();
            assert(count);// tree exists; assume there are *some* items
            WriteNumItems(writer, count);
            for (auto &&pair : tree) {
                writer.write(pair.first);
                writer.write(pair.second);
                //(std::tuple<> { pair.first, pair.second });
            }
            return true;
        }

        return false;
    }

    /*
    // Read a zdo_type from the DataStream
    template<typename T, typename CountType>
        requires is_member_v<T> && (std::same_as<CountType, char16_t> || std::same_as<CountType, std::uint8_t>)
    static void _TryReadType(DataReader& reader, member_map& members) {
        decltype(auto) count = reader.read<CountType>();

        for (int i = 0; i < count; i++) {
            // ...fuck
            // https://stackoverflow.com/questions/2934904/order-of-evaluation-in-c-function-parameters
            auto hash(reader.read<avledet::util::Hash>());
            auto type(reader.read<T>());
            _Set(hash, type, members);
        }
    }*/


    // TODO rename _Revise() ?
    void Revise()
    {
        this->m_rev.ReviseData();
    }

    void _SetPrefabHash(avledet::util::Hash hash)
    {
        this->m_prefabHash = hash;
    }

    // Set the owner of the ZDO without revising
    void _SetOwner(avledet::util::UserID owner)
    {
        ZDO_OWNERS[GetID()] = owner;
    }

    void _SetPosition(Vector3f pos)
    {
        this->m_pos = pos;
    }

    void _SetRotation(Vector3f rot)
    {
        this->m_rotation = rot;
    }

    void _SetRotation(Quaternion rot)
    {
        this->_SetRotation(rot.euler_angles());
    }


  private:
    /*
    * 36 bytes total:
    */
    ZDOID m_id;                                 // 4 bytes
    mutable Vector3f m_pos;                     // 12 bytes
    mutable ZDO::Rev m_rev;                     // 4 bytes
    mutable Vector3f m_rotation;                // 12 bytes
    mutable avledet::util::Hash m_prefabHash {};// 4 bytes

  public:
    ZDO(ZDOID id) :
        m_id(id)
    {
    }

    friend bool operator==(ZDOID const &lhs, ZDO const *rhs) noexcept
    {
        return lhs == rhs->GetID();
    }

    friend bool operator==(ZDOID const &lhs, std::unique_ptr<ZDO> const &rhs) noexcept
    {
        return lhs == rhs->GetID();
    }

    // Apply changes to ZDOManager
    //  make this a lua-only method?
    //bool Apply() const;

#if AVL_IS_ON(AVL_LEGACY_WORLD_LOADING)
    // Load ZDO from disk
    void Load31Pre(DataReader &reader, std::int32_t version);
#endif//AVL_LEGACY_WORLD_LOADING

    // Reads from a buffer using the new efficient format (version >= 31)
    //  version=0: Read according to the network deserialize format
    //  version>0: Read according to the file load format
    void Unpack(DataReader &reader, std::int32_t version);

    // Writes to a buffer using the new efficient format (version >= 31)
    //  If 'network' is true, write according to the network serialize format
    //  Otherwise write according to the file save format
    void Pack(DataWriter &writer, bool network) const;

    // TODO rename this to Remove (this has nearly the same functionality)
    // TODO add an extract that returns an optional (eliminate the T& out)
    // Erases and returns the value
    template<typename T>
    //requires is_member_v<T>
    static bool _Extract(VarMap<T> &map, ZDOID const &uid, avledet::util::Hash key, T &out)
    {
        auto &&find = map.find(uid);
        if (find != map.end()) {
            auto &&tree  = find->second;
            auto &&entry = tree.find(key);
            if (entry != tree.end()) {
                out = std::move(entry->second);
                tree.erase(entry);
                return true;
            }
        }
        return false;
    }

    template<typename T>
    //requires is_member_v<T>
    static bool _Extract(VarMap<T> &map, ZDOID const &uid, std::string_view key, T &out)
    {
        return _Extract(map, uid, avledet::util::get_stable_hash(key), out);
    }

    template<typename T>
        requires is_member_v<T>
    bool Extract(avledet::util::Hash key, T &out)
    {
        return _Extract(_GetVars<T>(), m_id, key, out);
    }

    template<typename T>
        requires is_member_v<T>
    bool Extract(std::string_view key, T &out)
    {
        return _Extract(_GetVars<T>(), m_id, key, out);
    }

    // Get a member by hash
    //  Returns null if absent
    //  Throws on type mismatch
    template<typename T>
    //requires is_member_v<T>
    [[nodiscard]] static T const *_Get(VarMap<T> const &map, ZDOID const &uid, avledet::util::Hash key)
    {
        auto &&find = map.find(uid);
        if (find != map.end()) {
            auto &&tree  = find->second;
            auto &&entry = tree.find(key);
            if (entry != tree.end()) {
                return &entry->second;
            }
        }
        return nullptr;
    }

    // Get a member by hash
    //  Returns null if absent
    //  Throws on type mismatch
    template<typename T>
    //requires is_member_v<T>
    [[nodiscard]] static T const *_Get(VarMap<T> const &map, ZDOID const &uid, std::string_view key)
    {
        return _Get(map, uid, avledet::util::get_stable_hash(key));
    }

    // Get a member by string
    //  Returns null if absent
    //  Throws on type mismatch
    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] T const *Get(avledet::util::Hash key) const
    {
        return _Get<T>(_GetVars<T>(), m_id, key);
    }

    // Get a member by string
    //  Returns null if absent
    //  Throws on type mismatch
    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] T const *Get(std::string_view key) const
    {
        return Get<T>(avledet::util::get_stable_hash(key));
    }

    // Trivial hash getters
    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] T const &Get(avledet::util::Hash key, T const &def) const
    {
        auto &&get = Get<T>(key);
        return get ? *get : def;
    }

    // Hash-key getters
    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] T const &Get(std::string_view key, T const &def) const
    {
        return Get<T>(avledet::util::get_stable_hash(key), def);
    }

    [[nodiscard]] float GetFloat(avledet::util::Hash key, float value) const
    {
        return Get<float>(key, value);
    }

    [[nodiscard]] std::int32_t GetInt(avledet::util::Hash key, std::int32_t value) const
    {
        return Get<std::int32_t>(key, value);
    }

    [[nodiscard]] std::int64_t GetLong(avledet::util::Hash key, std::int64_t value) const
    {
        return Get<std::int64_t>(key, value);
    }

    [[nodiscard]] Int64Wrapper GetLongWrapper(avledet::util::Hash key, Int64Wrapper value) const
    {
        return Get<std::int64_t>(key, value);
    }

    [[nodiscard]] Quaternion GetQuaternion(avledet::util::Hash key, Quaternion value) const
    {
        return Get<Quaternion>(key, value);
    }

    [[nodiscard]] Vector3f GetVector3(avledet::util::Hash key, Vector3f value) const
    {
        return Get<Vector3f>(key, value);
    }

    [[nodiscard]] std::string_view GetString(avledet::util::Hash key, std::string_view value) const
    {
        auto &&val = Get<std::string>(key);
        return val ? std::string_view(*val) : value;
    }

    [[nodiscard]] avledet::util::Bytes const *GetBytes(avledet::util::Hash key) const
    {
        return Get<avledet::util::Bytes>(key);
    }

    [[nodiscard]] bool GetBool(avledet::util::Hash key, bool value) const
    {
        return GetInt(key, value ? 1 : 0);
    }

    [[nodiscard]] ZDOID GetZDOID(std::pair<avledet::util::Hash, avledet::util::Hash> key, ZDOID value) const
    {
        return ZDOID(GetLong(key.first, value.get_user_id()), GetLong(key.second, value.get_id()));
    }

    // Hash-key default getters
    [[nodiscard]] float GetFloat(avledet::util::Hash key) const
    {
        return Get<float>(key, {});
    }

    [[nodiscard]] std::int32_t GetInt(avledet::util::Hash key) const
    {
        return Get<std::int32_t>(key, {});
    }

    [[nodiscard]] std::int64_t GetLong(avledet::util::Hash key) const
    {
        return Get<std::int64_t>(key, {});
    }

    [[nodiscard]] Int64Wrapper GetLongWrapper(avledet::util::Hash key) const
    {
        return Get<std::int64_t>(key, {});
    }

    [[nodiscard]] Quaternion GetQuaternion(avledet::util::Hash key) const
    {
        return Get<Quaternion>(key, {});
    }

    [[nodiscard]] Vector3f GetVector3(avledet::util::Hash key) const
    {
        return Get<Vector3f>(key, {});
    }

    [[nodiscard]] std::string_view GetString(avledet::util::Hash key) const
    {
        return Get<std::string>(key, {});
    }

    [[nodiscard]] bool GetBool(avledet::util::Hash key) const
    {
        return Get<std::int32_t>(key);
    }

    [[nodiscard]] ZDOID GetZDOID(std::pair<avledet::util::Hash, avledet::util::Hash> key) const
    {
        return GetZDOID(key, {});
    }

    // String-key getters
    [[nodiscard]] float GetFloat(std::string_view key, float value) const
    {
        return Get<float>(key, value);
    }

    [[nodiscard]] std::int32_t GetInt(std::string_view key, std::int32_t value) const
    {
        return Get<std::int32_t>(key, value);
    }

    [[nodiscard]] std::int64_t GetLong(std::string_view key, std::int64_t value) const
    {
        return Get<std::int64_t>(key, value);
    }

    [[nodiscard]] Int64Wrapper GetLongWrapper(std::string_view key, Int64Wrapper value) const
    {
        return Get<std::int64_t>(key, value);
    }

    [[nodiscard]] Quaternion GetQuaternion(std::string_view key, Quaternion value) const
    {
        return Get<Quaternion>(key, value);
    }

    [[nodiscard]] Vector3f GetVector3(std::string_view key, Vector3f value) const
    {
        return Get<Vector3f>(key, value);
    }

    [[nodiscard]] std::string_view GetString(std::string_view key, std::string_view value) const
    {
        auto &&val = Get<std::string>(key);
        return val ? std::string_view(*val) : value;
    }

    [[nodiscard]] avledet::util::Bytes const *GetBytes(std::string_view key) const
    {
        return Get<avledet::util::Bytes>(key);
    }

    [[nodiscard]] bool GetBool(std::string_view key, bool value) const
    {
        return Get<std::int32_t>(key, value);
    }

    [[nodiscard]] ZDOID GetZDOID(std::string_view key, ZDOID value) const
    {
        return GetZDOID(avledet::util::to_hash_pair(key), value);
    }

    // String-key default getters
    [[nodiscard]] float GetFloat(std::string_view key) const
    {
        return Get<float>(key, {});
    }

    [[nodiscard]] std::int32_t GetInt(std::string_view key) const
    {
        return Get<std::int32_t>(key, {});
    }

    [[nodiscard]] std::int64_t GetLong(std::string_view key) const
    {
        return Get<std::int64_t>(key, {});
    }

    [[nodiscard]] Int64Wrapper GetLongWrapper(std::string_view key) const
    {
        return Get<std::int64_t>(key, {});
    }

    [[nodiscard]] Quaternion GetQuaternion(std::string_view key) const
    {
        return Get<Quaternion>(key, {});
    }

    [[nodiscard]] Vector3f GetVector3(std::string_view key) const
    {
        return Get<Vector3f>(key, {});
    }

    [[nodiscard]] std::string_view GetString(std::string_view key) const
    {
        return Get<std::string>(key, {});
    }

    [[nodiscard]] bool GetBool(std::string_view key) const
    {
        return Get<std::int32_t>(key, {});
    }

    [[nodiscard]] ZDOID GetZDOID(std::string_view key) const
    {
        return GetZDOID(key, {});
    }

    // Trivial hash setters
    template<typename T>
        requires is_member_v<T>
    void Set(avledet::util::Hash key, T value)
    {
        if (_Set(key, std::move(value)))
            Revise();
    }

    // Special hash setters
    void Set(avledet::util::Hash key, bool value)
    {
        Set(key, value ? (std::int32_t) 1 : 0);
    }

    void Set(std::pair<avledet::util::Hash, avledet::util::Hash> const &key, ZDOID value)
    {
        Set(key.first, value.get_user_id());
        Set(key.second, (std::int64_t) value.get_id());
    }

    template<typename T>
        requires is_member_v<T>
    void Set(std::string_view key, T value)
    {
        Set(avledet::util::get_stable_hash(key), std::move(value));
    }

    void Set(std::string_view key, bool value)
    {
        Set(avledet::util::get_stable_hash(key), value ? (std::int32_t) 1 : 0);
    }

    void Set(std::string_view key, ZDOID value)
    {
        Set(avledet::util::to_hash_pair(key), value);
    }

    bool Extract(std::pair<avledet::util::Hash, avledet::util::Hash> key, ZDOID &out)
    {
        std::int64_t userID {};
        if (Extract(key.first, userID)) {
            std::int64_t id {};
            if (Extract(key.second, id)) {
                out = ZDOID(userID, id);
                return true;
            }
        }
        return false;
    }

    bool Extract(std::string_view key, ZDOID &out)
    {
        return Extract(avledet::util::to_hash_pair(key), out);
    }

    // Internal use
    //  Raw sets the connector with no revision
    bool _SetConnection(ZDOConnector::Type type, ZDOID zdoid)
    {
        auto &&insert = ZDO_TARGETED_CONNECTORS.insert({GetID(), ZDOConnectorTargeted(type, zdoid)});

        auto &&connector = insert.first->second;

        // if it was not newly inserted
        //  check the old values
        if (!insert.second) {
            auto &&type2  = connector.m_type;
            auto &&zdoid2 = connector.m_target;

            if (type == type2 && zdoid2 == zdoid) {
                return false;
            }
        }

        connector.m_type   = type;
        connector.m_target = zdoid;

        //m_pack.Merge<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Member_Connection));

        return true;
    }

    void SetConnection(ZDOConnector::Type type, ZDOID zdoid)
    {
        if (_SetConnection(type, zdoid)) {
            Revise();
        }
    }

    [[nodiscard]] ZDOID GetConnectionZDOID(ZDOConnector::Type type) const
    {
        auto &&find = ZDO_TARGETED_CONNECTORS.find(GetID());
        if (find != ZDO_TARGETED_CONNECTORS.end()) {
            if (find->second.m_type == type)
                return find->second.m_target;
        }
        return ZDOID::NONE;
    }

    [[nodiscard]] ZDOID GetID() const
    {
        return this->m_id;
    }

    [[nodiscard]] Vector3f GetPosition() const
    {
        return this->m_pos;
    }

    //void SetDataRevision(std::uint32_t dataRev) {
    //    m_data.get().m_rev.SetDataRevision(dataRev);
    //}
    //
    //void SetOwnerRevision(std::uint16_t ownerRev) {
    //    m_data.get().m_rev.SetOwnerRevision(ownerRev);
    //}


    Rev &GetRevision()
    {
        return this->m_rev;
    }

    // Set the position of the ZDO
    //  - Use this method 99.9% of the time when updating the ZDO's position
    //  - This will change and invalidate sectors if the new position is in a different zone than this ZDOs position
    void SetPosition(Vector3f pos);

    [[nodiscard]] ZoneID GetZone() const;

    [[nodiscard]] Quaternion GetRotation() const
    {
        return Quaternion::euler(this->m_rotation);
    }

    void SetRotation(Quaternion rot)
    {
        auto &&euler = rot.euler_angles();
        if (euler != this->m_rotation) {
            this->m_rotation = euler;
            this->Revise();
        }
    }

    [[nodiscard]] Prefab const &GetPrefab() const
    {
        return PrefabManager()->get_prefab(this->m_prefabHash);
    }

    [[nodiscard]] avledet::util::Hash GetPrefabHash() const
    {
        return this->m_prefabHash;
    }

    void SetLocalScale(Vector3f scale, bool allowIdentity)
    {
        // if scaling along all axis VS scaling axis differently
        // this is just to save some memory
        if (std::abs(scale.x - scale.y) < std::numeric_limits<float>::epsilon() * 8
            && std::abs(scale.y - scale.z) < std::numeric_limits<float>::epsilon() * 8) {

            if (allowIdentity || std::abs(scale.x - 1) > std::numeric_limits<float>::epsilon() * 8) {
                this->set(avledet::util::hashes::ZDO::ZNetView::SCALE_SCALAR, scale);
            }
        } else {
            // otherwise use scale
            this->set(avledet::util::hashes::ZDO::ZNetView::SCALE, scale);
        }
    }

    // The owner of the ZDO
    [[nodiscard]] avledet::util::UserID Owner() const
    {
        // TODO optimize by checking owner bit
        auto &&find = ZDO_OWNERS.find(GetID());
        if (find != ZDO_OWNERS.end()) {
            return find->second;
        }
        return 0;
    }

    // Whether the ZDO is owned by a specific owner
    [[nodiscard]] bool IsOwner(avledet::util::UserID owner) const
    {
        return owner == this->Owner();
    }

    // Returns whether this server is the owner of the ZDO
    [[nodiscard]] bool IsLocal() const
    {
        return this->IsOwner(AVL_ID);
    }

    // Whether the ZDO has an owner
    [[nodiscard]] bool HasOwner() const
    {
        return this->Owner() != 0;
        //return m_pack.Get<OWNER_PACK_INDEX>();
    }

    // Claim personal ownership over the ZDO
    bool SetLocal()
    {
        return this->SetOwner(AVL_ID);
    }

    // Clears the owner of this ZDO
    void Disown()
    {
        this->SetOwner(0);
    }

    // Set the owner of the ZDO
    bool SetOwner(avledet::util::UserID owner)
    {
        // only if the owner has changed, then revise it
        if (this->Owner() != owner) {
            this->_SetOwner(owner);

            this->m_rev.ReviseOwner();
            return true;
        }
        return false;
    }

    [[nodiscard]] std::uint16_t GetOwnerRevision() const
    {
        return this->m_rev.GetOwnerRevision();
    }

    [[nodiscard]] std::uint32_t GetDataRevision() const
    {
        return this->m_rev.GetDataRevision();
    }

    [[nodiscard]] bool IsPersistent() const
    {
        return GetPrefab().IsPersistent();
    }

    [[nodiscard]] bool IsDistant() const
    {
        return GetPrefab().IsDistant();
    }

    [[nodiscard]] avledet::util::ObjectType GetType() const
    {
        return GetPrefab().GetObjectType();
    }

    static bool is_sso(std::string const &str)
    {
        void const *strAddr  = static_cast<void const *>(&str);
        void const *dataAddr = static_cast<void const *>(str.data());

        bool addressCheck = std::fabs(reinterpret_cast<std::uintptr_t>(strAddr)
                                      - reinterpret_cast<std::uintptr_t>(dataAddr))
                            < sizeof(str);

        return addressCheck;
    }

    template<class T>
    static std::size_t GetTreeAlloc(Tree<T> &tree, bool full)
    {
        std::size_t var_count {};
        std::size_t extra_bytes {};

        std::size_t const unit_var_size = sizeof(T) + sizeof(std::int32_t);

        if (full) {
            if constexpr (std::is_same_v<T, std::string>) {
                for (auto &&[k1, v1] : tree) {
                    if (!is_sso(v1)) {//if string is heap allocated, then add this extra size
                        extra_bytes += v1.length();
                    }
                }
            } else if constexpr (std::is_same_v<T, std::vector<char>>) {
                for (auto &&[k1, v1] : tree) {
                    extra_bytes += v1.size();
                }
            }
        }

        return tree.size() * unit_var_size + extra_bytes;
    }

    template<class T>
    static std::size_t GetMapAlloc(bool full)
    {
        auto &&map = _GetVars<T>();

        std::size_t count_bytes {};
        for (auto &&[k, tree] : map) {
            count_bytes += GetTreeAlloc(tree, full);
        }

        return count_bytes + (sizeof(ZDOID) * map.size());

        /*
        std::size_t var_count {};

        auto&& map = _GetVars<T>();

        std::size_t extra_bytes {};
        const std::size_t unit_var_size = sizeof(T) + sizeof(std::int32_t);
        for (auto&& [k, tree] : map) {
            if (full) {
                if constexpr (std::is_same_v<T, std::string>) {
                    for (auto&& [k1, v1] : tree) {
                        if (!is_sso(v1)) { //if string is heap allocated, then add this extra size
                            extra_bytes += v1.length();
                        }
                    }
                } else if constexpr (std::is_same_v<T, std::vector<char>>) {
                    for (auto&& [k1, v1] : tree) {
                        extra_bytes += v1.size();
                    }
                }
            }

            var_count += tree.size();
        }

        return var_count * unit_var_size + (sizeof(ZDOID) * map.size()) + extra_bytes;
        */
    }

    static std::size_t GetTotalAlloc(bool full)
    {
        return GetMapAlloc<float>(full) + GetMapAlloc<avledet::util::CSU::Quaternion>(full)
               + GetMapAlloc<std::int32_t>(full) + GetMapAlloc<std::int64_t>(full)
               + GetMapAlloc<std::string>(full) + GetMapAlloc<std::vector<char>>(full)
               + GetMapAlloc<float>(full) + GetMapAlloc<float>(full)
               + ZDO_TARGETED_CONNECTORS.size()
                         * sizeof(decltype(ZDO_TARGETED_CONNECTORS)::value_type)//yes, pair  \/
               + ZDO_CONNECTORS.size() * sizeof(decltype(ZDO_CONNECTORS)::value_type)
               + ZDO_OWNERS.size() * sizeof(decltype(ZDO_OWNERS)::value_type);
    }

    /*
    [[nodiscard]] std::size_t GetTotalAlloc() const {
        //(decltype(m_floats)::bucket_type)
        std::size_t size = 0;

        //static inline VarMap<float> m_floats;
        //static inline VarMap<avledet::util::CSU::Vector3f> m_vec3;
        //static inline VarMap<avledet::util::CSU::Quaternion> m_quats;
        //static inline VarMap<std::int32_t> m_ints;
        //static inline VarMap<std::int64_t> m_longs;
        //static inline VarMap<std::string> m_strings;
        //static inline VarMap<std::vector<char>> m_byteArrays;
    //
        //static inline ankerl::unordered_dense::segmented_map<ZDOID, ZDOConnectorTargeted> ZDO_TARGETED_CONNECTORS; // Current linked connectors
        //static inline ankerl::unordered_dense::segmented_map<ZDOID, ZDOConnectorData> ZDO_CONNECTORS; // Saved typed-connectors
        //static inline ankerl::unordered_dense::segmented_map<ZDOID, std::int64_t> ZDO_OWNERS;
        
        auto&& find = ZDO_MEMBERS.find(GetID());
        if (find != ZDO_MEMBERS.end()) {
            for (auto&& member : find->second) {
                // TODO this only counts the compiled type size
                //  it does not include dynamically sized types like strings or arrays
                size += std::visit([](const auto& value) {
                    return sizeof(value);
                }, member.second);
            }
        }

        return size;
    }*/
};

namespace avledet::sync {
    using ZDO = ::ZDO;
}
