#pragma once

#include <algorithm>
#include <cmath>
#include <compare>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <optional>
#include <quill/LogMacros.h>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtl/btree.hpp>
#include <intrusive_shared_ptr/intrusive_shared_ptr.h>
#include <intrusive_shared_ptr/refcnt_ptr.h>

#include "Avledet.h"
#include "BitPack.h"
#include "DataStream.h"
#include "Hashes.h"
#include "PrefabManager.h"
#include "Quaternion.h"
#include "Replay.h"
#include "Types.h"
#include "Vector.h"
#include "VUtils.h"
#include "VUtilsMathf.h"
#include "ZDOConnector.h"
#include "ZDOID.h"

class ZDO
{
    // TODO are these friend classes safe?
    friend class ZdoManager;
    friend class PrefabManager;
    friend class Avledet;

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

    template<typename T>
    using is_member
            = VUtils::Traits::tuple_has_type<std::remove_cvref_t<T>,
                                             std::tuple<float, Vector3f, Quaternion, std::int32_t,
                                                        std::int64_t, std::string, std::vector<char>>>;

    template<typename T>
    static constexpr bool is_member_v = is_member<T>::value;

  public:
    class Rev
    {
      private:
        // DataRevision: 0, OwnerRevision: 1
        BitPack<std::uint32_t, 23, 32 - 23> m_pack;

        static constexpr auto DATA_REVISION_PACK_INDEX  = 0;
        static constexpr auto OWNER_REVISION_PACK_INDEX = 1;

      public:
        Rev();
        Rev(std::uint32_t dataRev, std::uint16_t ownerRev);

        std::uint32_t get_data_rev() const;
        std::uint16_t get_owner_rev() const;

        void set_data_rev(std::uint32_t dataRev);
        void set_owner_rev(std::uint16_t ownerRev);

        void rev_data();
        void rev_owner();
    };

  private:
    /*
    * 36 bytes total:
    */
    ZDOID m_id;                                         // 4 bytes
    mutable Vector3f m_pos;                             // 12 bytes
    mutable Rev m_rev;                                  // 4 bytes
    mutable Vector3f m_rotation;                        // 12 bytes
    mutable std::uint16_t m_refcnt {};                  // 2 bytes
    mutable std::uint16_t m_prefab_index = Prefab::NONE;// 2 bytes

  public:
    struct ptr_traits
    {
        static void add_ref(ZDO *ptr) noexcept
        {
            if (ptr->m_refcnt++ == std::numeric_limits<decltype(m_refcnt)>::max()) {
                assert(false);
                LOG_CRITICAL(AVL_LOGGER, "refcnt exceeded, this is very unusual: {}", ptr->m_id);
                std::exit(EXIT_FAILURE);
            }
        }

        static void sub_ref(ZDO *ptr) noexcept
        {
            assert(ptr->m_refcnt > 0);
            if (--ptr->m_refcnt == 0) {
                // free
                delete ptr;
            }
        }
    };

    using pointer   = ZDO *;
    using smart     = isptr::intrusive_shared_ptr<ZDO, ptr_traits>;
    using reference = smart;
    using optional  = smart;// considers null as empty

    /*
        OPERATOR <=>
    */
    friend auto operator<=>(smart const &lhs, smart const &rhs) noexcept
    {
        return lhs->m_id <=> rhs->m_id;
    }

    friend auto operator<=>(smart const &lhs, ZDOID const &rhs) noexcept
    {
        return lhs->m_id <=> rhs;
    }

    friend auto operator<=>(ZDO const &lhs, ZDO const &rhs) noexcept
    {
        return lhs.m_id <=> rhs.m_id;
    }

    /*
        OPERATOR ==
    */

    friend bool operator==(smart const &lhs, smart const &rhs) noexcept
    {
        return lhs->m_id == rhs->m_id;
    }

    friend bool operator==(smart const &lhs, ZDOID const &rhs) noexcept
    {
        return lhs->m_id == rhs;
    }

    friend bool operator==(ZDO const &lhs, ZDO const &rhs) noexcept
    {
        return lhs.m_id == rhs.m_id;
    }

    struct hash
    {
        using is_transparent = void;// enable heterogeneous overloads
        using is_avalanching = void;// mark class as high quality avalanching hash

        [[nodiscard]] auto operator()(pointer const &value) const noexcept -> std::uint64_t
        {
            assert(value);
            return ankerl::unordered_dense::hash<avledet::util::ZDOID> {}(value->m_id);
        }

        [[nodiscard]] auto operator()(smart const &value) const noexcept -> std::uint64_t
        {
            assert(value);
            return ankerl::unordered_dense::hash<avledet::util::ZDOID> {}(value->m_id);
        }

        [[nodiscard]] auto operator()(avledet::util::ZDOID const &value) const noexcept -> std::uint64_t
        {
            return ankerl::unordered_dense::hash<avledet::util::ZDOID> {}(value);
        }
    };

    struct equal_to
    {
        using is_transparent = void;

        template<typename T, typename U>
        bool operator()(T const &lhs, U const &rhs) const
        {
            return lhs == rhs;
        }
    };

    // smart: noref
    // soft: IDs stored
    // set/list...

    using smart_set      = avledet::util::Set<smart, hash, equal_to>;
    using soft_set       = avledet::util::Set<ZDOID, hash, std::equal_to<>>;// hetero hash?
    using reference_set  = avledet::util::Set<reference, hash, equal_to>;
    using reference_list = std::vector<reference>;
    using soft_list      = std::vector<ZDOID>;
    using Filter         = std::function<bool(reference)>;

    // Returns ref
    [[nodiscard]] static reference make_reference(smart_set::iterator itr)
    {
        return *itr;
    }

    // Returns ref
    [[nodiscard]] static reference make_reference(smart_set::value_type const &itr)
    {
        return itr;
    }

    // Returns new shared
    [[nodiscard]] static optional make_optional(reference v)
    {
        return v;
    }

    // Returns new shared
    [[nodiscard]] static optional make_optional(smart_set::iterator itr)
    {
        return *itr;
    }

    static inline auto const nullopt = nullptr;

  private:
    template<class T>
    using Tree = gtl::btree_map<avledet::util::Hash, T>;

    template<class T>
    using VarMap = ankerl::unordered_dense::segmented_map<avledet::util::ZDOID, Tree<T>, ZDO::hash,
                                                          std::equal_to<>>;

    static inline VarMap<float> m_floats;
    static inline VarMap<avledet::util::CSU::Vector3f> m_vec3;
    static inline VarMap<avledet::util::CSU::Quaternion> m_quats;
    static inline VarMap<std::int32_t> m_ints;
    static inline VarMap<std::int64_t> m_longs;
    static inline VarMap<std::string> m_strings;
    static inline VarMap<std::vector<char>> m_byteArrays;

    static inline ankerl::unordered_dense::segmented_map<ZDOID, ZDOConnectorTargeted>
            PAIRED_CONNECTORS;// Current linked connectors
    static inline ankerl::unordered_dense::segmented_map<ZDOID, ZDOConnectorData>
            TYPED_CONNECTORS; // Saved typed-connectors
    static inline ankerl::unordered_dense::segmented_map<ZDOID, std::int64_t> ZDO_OWNERS;

    /*
        Internal global tree getters
    */

    template<class T>
        requires is_member_v<T>
    static VarMap<T> &_get_vars()
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
    static Tree<T> *_find_var_tree(avledet::util::ZDOID const &uid)
    {
        auto &&map = _get_vars<T>();
        auto &&itr = map.find(uid);
        if (itr != map.end()) {
            return &itr->second;
        } else {
            return nullptr;
        }
    }

    template<class T>
    static std::pair<bool, std::reference_wrapper<Tree<T>>> _get_var_tree(avledet::util::ZDOID const &uid)
    {
        auto &&map = _get_vars<T>();
        auto &&emp = map.try_emplace(uid);
        return {emp.second, std::ref(emp.first->second)};
    }

    /*
        Converters for outdated versions
    */

    // TODO rename this to '_erase'
    template<typename T>
        requires is_member_v<T>
    static bool _extract(VarMap<T> &map, ZDOID const &uid, avledet::util::Hash key, T &out)
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
        requires is_member_v<T>
    static bool _extract(VarMap<T> &map, ZDOID const &uid, std::string_view key, T &out)
    {
        return _extract(map, uid, avledet::util::get_stable_hash(key), out);
    }

    static bool _can_strip(avledet::util::Hash key);
    static bool _can_strip(avledet::util::Hash key, float data);
    static bool _can_strip(avledet::util::Hash key, avledet::util::CSU::Quaternion const &data);
    static bool _can_strip(avledet::util::Hash key, std::int32_t data);
    static bool _can_strip(avledet::util::Hash key, std::int64_t data);
    static bool _can_strip(avledet::util::Hash key, std::string const &data);
    static bool _can_strip(avledet::util::Hash key, std::vector<char> const &data);

    // (Keep as a member function, to access m_id as needed in future)
    template<class T>
        requires(!std::is_same_v<T, avledet::util::CSU::Vector3f>)
    bool _try_convert(avledet::util::Hash key, T const &data)
    {
        return _can_strip(key, data);
    }

    // (Keep as a member function, to access m_id as needed in future)
    bool _try_convert(avledet::util::Hash key, avledet::util::CSU::Vector3f data);

    static std::uint32_t _read_num_items(avledet::util::Reader &reader, int version);

    static void _write_num_items(DataWriter &writer, int numItems);

    //template<class T>
    //void _load_vars(avledet::util::Reader &reader, int version, VarMap<T> &map)
    template<bool do_replay_compile>
    void _load_vars(avledet::util::Reader &reader, int version, VarMap<avledet::util::Bytes> &map, avledet::replay::DataTracker *tracker)
    {
        // dumb temp to help my intellisense
        using T = avledet::util::Bytes;

        auto num3     = _read_num_items(reader, version);
        auto &&insert = map.try_emplace(m_id);
        auto &&tree   = insert.first->second;
        for (decltype(num3) i = 0; i < num3; i++) {
            int num4  = reader.read<avledet::util::Hash>();
            auto num5 = reader.read<T>();
            // Run during network, or if conversion during world-load
            if (!(version && _try_convert(num4, num5))) {
                // TODO, this inefficient now, but, do later insert/replace...
                //auto&& item = tree[num4];
                if constexpr (do_replay_compile) {
                    auto&& find = tree.find(num4);
                    if (find != tree.end()) {
                        if (find->second != num5) {
                            // then append to replay change set

                            // TODO need to somehow 
                            tracker->compile()
                        }
                        
                        //item = num5;
                    }
                }
                tree[num4] = num5;
            }
        }

        if (tree.empty()) {
            map.erase(insert.first);

            // !!!WARNING!!! do NOT access 'insert' or 'tree'!
        }
    }

    template<typename T>
        requires is_member_v<T>
    decltype(auto) _try_write_type(DataWriter &writer) const
    {
        auto &&tree_ptr = _find_var_tree<T>(m_id);
        if (tree_ptr) {
            auto &&tree      = *tree_ptr;
            auto const count = tree.size();
            assert(count);// tree exists; assume there are *some* items
            _write_num_items(writer, count);
            for (auto &&pair : tree) {
                writer.write(pair.first);
                writer.write(pair.second);
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

  private:
    /*
        Internal data setters
    */

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

    template<class T>
    static bool _set(avledet::util::ZDOID const &uid, avledet::util::Hash key, T data)
    {
        auto &&[inserted, tree] = _get_var_tree<T>(uid);
        return _set(tree.get(), key, std::move(data)) || inserted;
    }

    template<typename T>
        requires is_member_v<T>
    [[maybe_unused]] bool _set(avledet::util::Hash key, T value)
    {
        return _set(m_id, key, std::move(value));
    }

    bool _set_connection(ZDOConnector::Type type, ZDOID zdoid);

    // Get a member by hash
    //  Returns null if absent
    //  Throws on type mismatch
    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] static T const *_find(VarMap<T> const &map, ZDOID const &uid, avledet::util::Hash key)
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
        requires is_member_v<T>
    [[nodiscard]] static T const *_find(VarMap<T> const &map, ZDOID const &uid, std::string_view key)
    {
        return _find(map, uid, avledet::util::get_stable_hash(key));
    }

    /*
        Internal member setters (unrevised)
    */

    void _revise();

    void _set_prefab_hash(avledet::util::Hash hash);

    void _set_owner(avledet::util::UserID owner);

    void _set_position(Vector3f const &pos);

    void _set_rotation(Vector3f const &rot);

    void _set_rotation(Quaternion const &rot);

  private:
    ZDO(ZDOID id) :
        m_id(id)
    {
    }

    // similar to shared_from_this()
    smart smart_from_this()
    {
        return smart::ref(this);
    }

    // similar to make_shared()
    template<class... Args>
    static smart make_smart(Args &&...args)
    {
        //return smart::noref(new ZDO(std::forward<Args>(args)...));
        //https://github.com/gershnik/intrusive_shared_ptr?tab=readme-ov-file#using-provided-base-classes
        // "ATTACH" uses "noref", which "CLAIMS" to create with refcnt of '1'
        //  this is UNTRUE
        //  directly via the code, noref does NOTHING, except for encapsulate the raw pointer
        //  which results in the pointer leaking memory when the last wrapper is destroyed (this)
        return smart::ref(new ZDO(std::forward<Args>(args)...));
    }

  public:
#if AVL_IS_ON(AVL_LEGACY_WORLD_LOADING)
    // Load ZDO from disk
    void Load31Pre(DataReader &reader, std::int32_t version);
#endif//AVL_LEGACY_WORLD_LOADING

    // Reads from a buffer using the new efficient format (version >= 31)
    //  version=0: Read according to the network deserialize format
    //  version>0: Read according to the file load format
    void unpack(DataReader &reader, std::int32_t version);

    // Writes to a buffer using the new efficient format (version >= 31)
    //  If 'network' is true, write according to the network serialize format
    //  Otherwise write according to the file save format
    void pack(DataWriter &writer, bool network) const;

    // Get a member by hash
    //  Returns null if absent
    //  Throws on type mismatch
    //template<typename T>
    //    requires is_member_v<T>
    //[[nodiscard]] static decltype(auto) _find_vars(VarMap<T> const &map, ZDOID const &uid, avledet::util::Hash key)
    //{
    //    auto &&find = map.find(uid);
    //    if (find != map.end()) {
    //        auto &&tree  = find->second;
    //        auto &&entry = tree.find(key);
    //        if (entry != tree.end()) {
    //            return entry;
    //        }
    //    }
    //    return std::make_tuple(nullptr,;
    //}

    //VarMap<std::string>::

    /*
        Data removers
    */

    // Remove type from vars
    // (not safe while looping members)
    template<typename T>
        requires is_member_v<T>
    bool extract(avledet::util::Hash key, T &out)
    {
        return _extract(_get_vars<T>(), m_id, key, out);
    }

    // Remove type from vars
    // (not safe while looping members)
    template<typename T>
        requires is_member_v<T>
    bool extract(std::string_view key, T &out)
    {
        return _extract(_get_vars<T>(), m_id, key, out);
    }

    // Remove type from vars
    // (not safe while looping members)
    template<typename T>
        requires is_member_v<T>
    std::optional<T> extract(std::string_view key)
    {
        T out;
        if (extract(key, out)) {
            return out;
        }
        return std::nullopt;
    }

    // Remove type from vars (ZDOID overload)
    // (not safe while looping members)
    bool extract(std::pair<avledet::util::Hash, avledet::util::Hash> key, ZDOID &out)
    {
        std::int64_t userID {};
        if (this->extract(key.first, userID)) {
            std::int64_t id {};
            if (this->extract(key.second, id)) {
                // TODO ensure that id fits within a uint
                assert(false);
                out = ZDOID(userID, (std::uint32_t)id);
                return true;
            }
        }
        return false;
    }

    // Remove type from vars (ZDOID overload)
    // (not safe while looping members)
    bool extract(std::string_view key, ZDOID &out)
    {
        return extract(avledet::util::to_hash_pair(key), out);
    }

    std::optional<ZDOID> extract(std::pair<avledet::util::Hash, avledet::util::Hash> key)
    {
        ZDOID out;
        if (extract(key, out)) {
            return out;
        }
        return std::nullopt;
    }

    // Remove type from vars (ZDOID overload)
    // (not safe while looping members)
    std::optional<ZDOID> extract(std::string_view key)
    {
        return extract(avledet::util::to_hash_pair(key));
    }

    // Get a member by string
    //  Returns null if absent
    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] T const *find(avledet::util::Hash key) const
    {
        return _find(_get_vars<T>(), m_id, key);
    }

    // Get a member by string
    //  Returns null if absent
    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] T const *find(std::string_view key) const
    {
        return find<T>(avledet::util::get_stable_hash(key));
    }

    // Trivial hash getters
    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] T const &get(avledet::util::Hash key, T const &def) const
    {
        auto &&get = find<T>(key);
        return get ? *get : def;
    }

    /*******************
        Hash getters
    *******************/

    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] T const &get(std::string_view key, T const &def) const
    {
        return get<T>(avledet::util::get_stable_hash(key), def);
    }

    [[nodiscard]] float get_float(avledet::util::Hash key, float value) const
    {
        return get<float>(key, value);
    }

    [[nodiscard]] std::int32_t get_int(avledet::util::Hash key, std::int32_t value) const
    {
        return get<std::int32_t>(key, value);
    }

    [[nodiscard]] std::int64_t get_long(avledet::util::Hash key, std::int64_t value) const
    {
        return get<std::int64_t>(key, value);
    }

    [[nodiscard]] Quaternion get_quat(avledet::util::Hash key, Quaternion value) const
    {
        return get<Quaternion>(key, value);
    }

    [[nodiscard]] Vector3f get_vec3(avledet::util::Hash key, Vector3f value) const
    {
        return get<Vector3f>(key, value);
    }

    [[nodiscard]] std::string_view get_string(avledet::util::Hash key, std::string_view value) const
    {
        auto &&val = find<std::string>(key);
        return val ? std::string_view(*val) : value;
    }

    // TODO return view
    [[nodiscard]] avledet::util::Bytes const *find_bytes(avledet::util::Hash key) const
    {
        return find<avledet::util::Bytes>(key);
    }

    [[nodiscard]] bool get_bool(avledet::util::Hash key, bool value) const
    {
        return get_int(key, value ? 1 : 0);
    }

    [[nodiscard]] ZDOID get_zdoid(std::pair<avledet::util::Hash, avledet::util::Hash> key, ZDOID value) const
    {
        assert(false);
        // TODO check size of 'id' fits within int32
        return ZDOID(get_long(key.first, value.get_user_id()), (std::int32_t)get_long(key.second, value.get_id()));
    }

    /*******************
    Hash getters (default)
    *******************/

    [[nodiscard]] float get_float(avledet::util::Hash key) const
    {
        return get<float>(key, {});
    }

    [[nodiscard]] std::int32_t get_int(avledet::util::Hash key) const
    {
        return get<std::int32_t>(key, {});
    }

    [[nodiscard]] std::int64_t get_long(avledet::util::Hash key) const
    {
        return get<std::int64_t>(key, {});
    }

    [[nodiscard]] Quaternion get_quat(avledet::util::Hash key) const
    {
        return get<Quaternion>(key, {});
    }

    [[nodiscard]] Vector3f get_vec3(avledet::util::Hash key) const
    {
        return get<Vector3f>(key, {});
    }

    [[nodiscard]] std::string_view get_string(avledet::util::Hash key) const
    {
        return get<std::string>(key, {});
    }

    [[nodiscard]] bool get_bool(avledet::util::Hash key) const
    {
        return get<std::int32_t>(key, {});
    }

    [[nodiscard]] ZDOID get_zdoid(std::pair<avledet::util::Hash, avledet::util::Hash> key) const
    {
        return get_zdoid(key, {});
    }

    /*******************
     String-key getters
    *******************/

    [[nodiscard]] float get_float(std::string_view key, float value) const
    {
        return get<float>(key, value);
    }

    [[nodiscard]] std::int32_t get_int(std::string_view key, std::int32_t value) const
    {
        return get<std::int32_t>(key, value);
    }

    [[nodiscard]] std::int64_t get_long(std::string_view key, std::int64_t value) const
    {
        return get<std::int64_t>(key, value);
    }

    [[nodiscard]] Quaternion get_quat(std::string_view key, Quaternion value) const
    {
        return get<Quaternion>(key, value);
    }

    [[nodiscard]] Vector3f get_vec3(std::string_view key, Vector3f value) const
    {
        return get<Vector3f>(key, value);
    }

    [[nodiscard]] std::string_view get_string(std::string_view key, std::string_view value) const
    {
        auto &&val = find<std::string>(key);
        return val ? std::string_view(*val) : value;
    }

    [[nodiscard]] avledet::util::Bytes const *find_bytes(std::string_view key) const
    {
        return find<avledet::util::Bytes>(key);
    }

    [[nodiscard]] bool get_bool(std::string_view key, bool value) const
    {
        return get<std::int32_t>(key, value);
    }

    [[nodiscard]] ZDOID get_zdoid(std::string_view key, ZDOID value) const
    {
        return get_zdoid(avledet::util::to_hash_pair(key), value);
    }

    /*******************
     String getters (default)
    *******************/

    [[nodiscard]] float get_float(std::string_view key) const
    {
        return get<float>(key, {});
    }

    [[nodiscard]] std::int32_t get_int(std::string_view key) const
    {
        return get<std::int32_t>(key, {});
    }

    [[nodiscard]] std::int64_t get_long(std::string_view key) const
    {
        return get<std::int64_t>(key, {});
    }

    [[nodiscard]] Quaternion get_quat(std::string_view key) const
    {
        return get<Quaternion>(key, {});
    }

    [[nodiscard]] Vector3f get_vec3(std::string_view key) const
    {
        return get<Vector3f>(key, {});
    }

    [[nodiscard]] std::string_view get_string(std::string_view key) const
    {
        return get<std::string>(key, {});
    }

    [[nodiscard]] bool get_bool(std::string_view key) const
    {
        return get<std::int32_t>(key, {});
    }

    [[nodiscard]] ZDOID get_zdoid(std::string_view key) const
    {
        return get_zdoid(key, {});
    }

    /*
        Hash setters
    */

    template<class T>
        requires is_member_v<T>
    bool set(avledet::util::Hash key, T data)
    {
        if (_set(key, std::move(data))) {
            _revise();
            return true;
        }
        return false;
    }

    // Special hash setters
    bool set(avledet::util::Hash key, bool value)
    {
        return set(key, value ? (std::int32_t) 1 : 0);
    }

    bool set(std::pair<avledet::util::Hash, avledet::util::Hash> const &key, ZDOID value)
    {
        bool a = set(key.first, value.get_user_id());
        bool b = set(key.second, (std::int64_t) value.get_id());
        return a || b;
    }

    // TODO remove?
    //template<class T>
    //bool set(std::string_view key, T data)
    //{
    //    return set(avledet::util::get_stable_hash(key), std::move(data));
    //}

    template<typename T>
    bool set(std::string_view key, T value)
    {
        return set(avledet::util::get_stable_hash(key), std::move(value));
    }

    bool set(std::string_view key, bool value)
    {
        return set(avledet::util::get_stable_hash(key), value);
    }

    bool set(std::string_view key, ZDOID value)
    {
        return set(avledet::util::to_hash_pair(key), value);
    }

    // Internal use; (mutable revision for use by RPC_ZDOData lambda)
    //Rev &_get_revision();

    /*
        Member accessors
    */

    void set_connection(ZDOConnector::Type type, ZDOID zdoid);

    ZDOID get_connection_zdoid(ZDOConnector::Type type) const;

    ZDOID get_id() const;

    Vector3f get_position() const;

    void set_position(Vector3f pos);

    avledet::util::ZoneID get_zone() const;

    Quaternion get_rotation() const;

    void set_rotation(Quaternion rot);

    Prefab::Reference get_prefab() const;

    avledet::util::Hash get_prefab_hash() const;

    void set_local_scale(Vector3f scale, bool allowIdentity);

    // The owner of the ZDO
    avledet::util::UserID get_owner() const;

    // Whether the ZDO is owned by
    bool is_owner(avledet::util::UserID owner) const;

    // Whether im the owner
    bool owned_by_me() const;

    // Whether the ZDO has an owner
    bool has_owner() const;

    // Claim personal ownership over the ZDO
    bool claim();

    void set_claimed(bool local);

    // Clears the owner of this ZDO
    void disown();

    // Set the owner of the ZDO
    bool set_owner(avledet::util::UserID owner);

    std::uint16_t get_owner_rev() const;

    std::uint32_t get_data_rev() const;

    bool is_persistent() const;

    bool is_distant() const;

    avledet::util::ObjectType get_type() const;

    /*
        Memory polling utilities
    */

    template<class T>
    static std::size_t get_tree_memory(Tree<T> &tree, bool full)
    {
        std::size_t var_count {};
        std::size_t extra_bytes {};

        std::size_t const unit_var_size = sizeof(T) + sizeof(std::int32_t);

        if (full) {
            if constexpr (std::is_same_v<T, std::string>) {
                for (auto &&[k1, v1] : tree) {
                    auto is_sso = [](auto &&str) {
                        void const *strAddr  = static_cast<void const *>(&str);
                        void const *dataAddr = static_cast<void const *>(str.data());

                        bool addressCheck = std::fabs(reinterpret_cast<std::uintptr_t>(strAddr)
                                                      - reinterpret_cast<std::uintptr_t>(dataAddr))
                                            < sizeof(str);

                        return addressCheck;
                    };

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
    static std::size_t get_memory_vars(bool full)
    {
        auto &&map = _get_vars<T>();

        std::size_t count_bytes {};
        for (auto &&[k, tree] : map) {
            count_bytes += get_tree_memory(tree, full);
        }

        return count_bytes + (sizeof(ZDOID) * map.size());
    }

    static std::size_t get_memory(bool full)
    {
        return get_memory_vars<float>(full) + get_memory_vars<avledet::util::CSU::Quaternion>(full)
               + get_memory_vars<std::int32_t>(full) + get_memory_vars<std::int64_t>(full)
               + get_memory_vars<std::string>(full) + get_memory_vars<std::vector<char>>(full)
               + get_memory_vars<float>(full) + get_memory_vars<float>(full)
               + PAIRED_CONNECTORS.size() * sizeof(decltype(PAIRED_CONNECTORS)::value_type)//yes, pair  \/
               + TYPED_CONNECTORS.size() * sizeof(decltype(TYPED_CONNECTORS)::value_type)
               + ZDO_OWNERS.size() * sizeof(decltype(ZDO_OWNERS)::value_type);
    }

    friend std::ostream &operator<<(std::ostream &ostr, ZDO const &value)
    {
        return ostr << value.m_id;
    }
};

namespace avledet::sync {
    using ZDO = ::ZDO;
}
