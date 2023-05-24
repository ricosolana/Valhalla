#pragma once

#include <type_traits>
#include <algorithm>

#include "VUtils.h"
#include "VUtilsTraits.h"
#include "VUtilsString.h"
#include "BitPack.h"
#include "Hashes.h"
#include "HashUtils.h"
#include "Quaternion.h"
#include "Vector.h"
#include "DataWriter.h"
#include "DataReader.h"
#include "ValhallaServer.h"
#include "ZoneManager.h"
#include "PrefabManager.h"
#include "ZDOConnector.h"

// The 'butter' of Valheim
// This class has been refactored numerous times 
//  Performance is important but memory usage has been highly prioritized here
// This class used to be 500+ bytes
//  It is now 120 bytes 
// This class is finally the smallest it could possibly be (I hope so).
//  I Lied this class is now
class ZDO {
    friend class IZDOManager;
    friend class IPrefabManager;
    friend class Tests;
    friend class IValhalla;



    //using zdo_local_mask = uint8_t;

    using zdo_global_mask = uint16_t;
    using zdo_xhash = uint64_t;

    using zdo_member_tuple = std::tuple<float, Vector3f, Quaternion, int32_t, int64_t, std::string, BYTES_t>;
    using zdo_member_variant = VUtils::Traits::tuple_to_variant<zdo_member_tuple>::type;

    // TODO fix this templated thing (zdo_member_variant not working)
    using zdo_member_map = UNORDERED_MAP_t<zdo_xhash, zdo_member_variant>;
    //using zdo_member_map = UNORDERED_MAP_t<zdo_xhash, std::variant<float, Vector3f, Quaternion, int32_t, int64_t, std::string, BYTES_t>>;

    template<typename T>
    struct is_zdo_member : VUtils::Traits::tuple_has_type<T, zdo_member_tuple> {};

    template<typename T>
        requires is_zdo_member<T>::value
    static constexpr decltype(auto) GetMemberDenotion() {
        return LocalDenotion(VUtils::Traits::tuple_index<T, zdo_member_tuple>::value);
    }

public:
    class Rev {
    private:
        //static constexpr uint32_t LEADING_OWNER_BITS = 21;

        // DataRevision: 0, OwnerRevision: 1
        BitPack<uint32_t, 21, 32 - 21> m_pack;

        //uint32_t m_encoded{};
        
        //static constexpr decltype(m_encoded) OWNER_MASK = (static_cast<decltype(m_encoded)>(1) << LEADING_OWNER_BITS) - 1;

    public:
        Rev() {}

        Rev(uint32_t dataRev, uint16_t ownerRev) {
            SetDataRevision(dataRev);
            SetOwnerRevision(ownerRev);
        }

        uint32_t GetDataRevision() const {
            return m_pack.Get<0>();
        }

        uint16_t GetOwnerRevision() const {
            return m_pack.Get<1>();
        }



        void SetDataRevision(uint32_t dataRev) {
            m_pack.Set<0>(dataRev);
        }

        void SetOwnerRevision(uint16_t ownerRev) {
            m_pack.Set<1>(ownerRev);
        }



        void ReviseData() {
            this->SetDataRevision(GetDataRevision() + 1);
        }

        void ReviseOwner() {
            this->SetOwnerRevision(GetOwnerRevision() + 1);
        }
    };

    static std::pair<HASH_t, HASH_t> ToHashPair(std::string_view key) {
        return {
            VUtils::String::GetStableHashCode(std::string(key) + "_u"),
            VUtils::String::GetStableHashCode(std::string(key) + "_i")
        };
    }

private:
    enum class LocalDenotion {
        Member_Connection,
        Member_Float,
        Member_Vec3,
        Member_Quat,
        Member_Int,
        Member_Long,
        Member_String,
        Member_ByteArray,
        //Marker_Owner,
        Marker_Persistent,
        Marker_Distant,
        Marker_Type1,
        Marker_Type2,
    };

    // Valheim specific enum for both file/network in newer efficient version
    enum class GlobalDenotion : zdo_global_mask {
        Member_Connection,
        Member_Float,
        Member_Vec3,
        Member_Quat,
        Member_Int,
        Member_Long,
        Member_String,
        Member_ByteArray,
        Marker_Persistent,
        Marker_Distant,
        Marker_Type1,
        Marker_Type2,
        Marker_Rotation,
    };

    enum class LocalFlag {
        None = 0,
        Member_Connection = 1 << 0,
        Member_Float = 1 << 1,
        Member_Vec3 = 1 << 2,
        Member_Quat = 1 << 3,
        Member_Int = 1 << 4,
        Member_Long = 1 << 5,
        Member_String = 1 << 6,
        Member_ByteArray = 1 << 7,
        //Marker_Owner = 1 << 8,
        Marker_Persistent = 1 << 8,
        Marker_Distant = 1 << 9,
        Marker_Type1 = 1 << 10,
        Marker_Type2 = 1 << 11,
    };

    // Valheim specific flags for both file/network in newer efficient version
    enum class GlobalFlag : zdo_global_mask {
        None = 0,
        Member_Connection = 1 << 0,
        Member_Float = 1 << 1,
        Member_Vec3 = 1 << 2,
        Member_Quat = 1 << 3,
        Member_Int = 1 << 4,
        Member_Long = 1 << 5,
        Member_String = 1 << 6,
        Member_ByteArray = 1 << 7,
        Marker_Persistent = 1 << 8,
        Marker_Distant = 1 << 9,
        Marker_Type1 = 1 << 10,
        Marker_Type2 = 1 << 11,
        Marker_Rotation = 1 << 12,
    };

private:

    // Set the object by hash (Internal use only; does not revise ZDO on changes)
    //  Returns whether the previous value was modified
    //  Throws on type mismatch
    template<typename T>
        requires is_zdo_member<T>::value
    bool _Set(HASH_t key, T value, zdo_member_map& members) {
        auto mut = hash_to_xhash<T>(key);

        //auto&& members = ZDO_MEMBERS[ID()];

        static_assert(GetMemberDenotion<float>() + 1 == LocalDenotion::Member_Float)

        auto&& insert = members.insert({ mut, 0.f });
        if (insert.second) {
            // Then officially assign
            insert.first->second = std::move(value);
            //m_encoded.AddDenotion(GetMemberDenotion<T>());
            m_pack.Merge<2>(1 << (GetMemberDenotion<T>() + 1));
            return true;
        }
        else {
            assert(m_pack.Get<2>() & ((1 << GetMemberDenotion<T>()) + 1));

            // else try modifying it ONLY if the member is same type
            auto&& get = std::get_if<T>(&insert.first->second);
            if (get) {
                if (!std::is_fundamental_v<T>
                    || *get != value)
                {
                    *get = std::move(value);
                    return true;
                }
                return false;
            }
            else {
                throw std::runtime_error("zdo member type collision; this is very rare; consult a doctor");
            }
        }
    }

    template<typename T> 
        requires is_zdo_member<T>::value
    bool _Set(HASH_t key, T value) {
        return _Set(key, std::move(value), ZDO_MEMBERS[ID()]);
    }

    void Revise() {
        m_rev.SetDataRevision(m_rev.GetDataRevision() + 1);
    }

    template<typename T>
        requires is_zdo_member<T>::value
    zdo_xhash hash_to_xhash(HASH_t in) const {
        return static_cast<zdo_xhash>(in) 
            ^ static_cast<zdo_xhash>(ankerl::unordered_dense::hash<size_t>{}(VUtils::Traits::tuple_index<T, zdo_member_tuple>::value));
    }

    template<typename T>
        requires is_zdo_member<T>::value
    HASH_t xhash_to_hash(zdo_xhash in) const {
        return static_cast<HASH_t>(in) 
            ^ static_cast<HASH_t>(ankerl::unordered_dense::hash<size_t>{}(VUtils::Traits::tuple_index<T, zdo_member_tuple>::value));
    }

    template<typename T>
        requires is_zdo_member<T>::value
    void _TryWriteType(DataWriter& writer, zdo_member_map& members) const {
        if (m_pack.Get<2>() & (1 << GetMemberDenotion<T>())) {
        //if (m_encoded.HasMember<T>()) {     
            const auto begin_mark = writer.Position();
            uint8_t count = 0;
            writer.Write(count); // placeholder 0 byte

            for (auto&& pair : members) {
                auto&& data = std::get_if<T>(&pair.second);
                if (data) {
                    writer.Write(this->xhash_to_hash<T>(pair.first));
                    writer.Write(*data);
                    count++;
                }
            }

            if (count) {
                auto end_mark = writer.Position();
                writer.SetPos(begin_mark);
                writer.Write(count);
                writer.SetPos(end_mark);
            }
        }
    }

    // Read a zdo_type from the DataStream
    template<typename T, typename CountType>
        requires std::same_as<CountType, char16_t> || std::same_as<CountType, uint8_t>
    void _TryReadType(DataReader& reader, zdo_member_map& members) {
        decltype(auto) count = reader.Read<CountType>();

        for (int i=0; i < count; i++) {
            // ...fuck
            // https://stackoverflow.com/questions/2934904/order-of-evaluation-in-c-function-parameters
            auto hash(reader.Read<HASH_t>());
            auto type(reader.Read<T>());
            _Set(hash, type, members);
        }
    }



private:
    static ankerl::unordered_dense::segmented_map<ZDOID, zdo_member_map> ZDO_MEMBERS;
    static ankerl::unordered_dense::segmented_map<ZDOID, ZDOConnectorData> ZDO_CONNECTORS; // Saved typed-connectors
    static ankerl::unordered_dense::segmented_map<ZDOID, ZDOConnectorTargeted> ZDO_TARGETED_CONNECTORS; // Current linked connectors

    /*
    * 36 bytes total:
    */

    Vector3f m_pos;                                 // 12 bytes
    ZDOID m_id;                                     // 4 bytes (PADDING)
    Vector3f m_rotation;                            // 12 bytes
    Rev m_rev;                                      // 4 bytes
    // Owner: 0, Prefab: 1, Flags: 2
    BitPack<uint32_t, decltype(ZDOID::m_pack)::count<0>::value, 12, 12, 2> m_pack;

    /*
    // Encoding of PREFAB, FLAGS, OWNER
    //  pppppppp ppppffff ffffffff nnuuuuuu
    //  p: PREFAB, f: FLAGS, n: UNUSED, u: OWNER
    class _zdoEnc {
    private:
        static constexpr uint32_t OWNER_BIT_OFFSET = 0;
        static constexpr uint32_t OWNER_BIT_COUNT = ZDOID::USER_BIT_COUNT;

        static constexpr uint32_t FLAG_BIT_OFFSET = 8;
        static constexpr uint32_t FLAG_BIT_COUNT = 12;

        static constexpr uint32_t PREFAB_BIT_OFFSET = 20;
        static constexpr uint32_t PREFAB_BIT_COUNT = 12;

    private:
        uint32_t m_data{};

    private:
        // Retrive a number by bit offset and bit count from m_data
        constexpr decltype(m_data) Get(uint8_t offset, uint8_t count) const {
            return (m_data >> offset) & ((1 << count) - 1);
        }

        constexpr void Add(decltype(m_data) value, uint8_t offset, uint8_t count) {
            m_data |= (value & ((1 << count) - 1)) << offset;
        }

        constexpr void Set(decltype(m_data) value, uint8_t offset, uint8_t count) {
            assert((value <= static_cast<decltype(m_data)>((1 << count) - 1)) && "data loss");

            // zero out
            m_data &= ~((1 << count) - 1) << offset;

            // merge portion
            Add(value, offset, count);
        }

    public:
        constexpr _zdoEnc() {}

        constexpr _zdoEnc(const _zdoEnc& other) = default;
        constexpr _zdoEnc(_zdoEnc&& other) {
            this->m_data = other.m_data;
            other.m_data = 0;
        }

        constexpr void operator=(const _zdoEnc& other) {
            this->m_data = other.m_data;
        }

        // Get all machine-representative flags
        constexpr LocalFlag GetFlags() const {
            return (LocalFlag) Get(FLAG_BIT_OFFSET, FLAG_BIT_COUNT);
        }

        // Check whether this ZDO has at least all of the specified flags
        //  Flags may be xored together
        bool HasAllFlags(LocalFlag flags) const {
            return (GetFlags() & flags) == flags;
        }

        // Check whether this ZDO has any of the specified flags
        //  Flags may be xored together
        bool HasAnyFlags(LocalFlag flags) const {
            return (GetFlags() & flags) != LocalFlag::None;
        }

        constexpr void AddFlags(LocalFlag flags) {
            Add(std::to_underlying(flags), FLAG_BIT_OFFSET, FLAG_BIT_COUNT);
        }

        // Check whether this ZDO has any of the specified data types
        
        bool HasDenotion(LocalDenotion denotion) const {
            return HasAllFlags(LocalFlag(1 << denotion));
        }

        constexpr void AddDenotion(LocalDenotion denotion) {
            AddFlags(LocalFlag(1 << denotion));
        }

        // Check whether this ZDO contains a specific templated member data type
        template<typename T>
            requires is_zdo_member<T>::value
        bool HasMember() const {
            static_assert(
                VUtils::Traits::tuple_index<float, zdo_member_tuple>::value + 1 == std::to_underlying(LocalDenotion::Member_Float),
                "zdo_types type indexes must must zdo enum Flags"
                );

            return HasDenotion(LocalDenotion(VUtils::Traits::tuple_index<T, zdo_member_tuple>::value + 1));
        }



        constexpr decltype(auto) GetPrefabIndex() const {
            return Get(PREFAB_BIT_OFFSET, PREFAB_BIT_COUNT);
        }

        constexpr void SetPrefabIndex(decltype(m_data) index) {
            Set(index, PREFAB_BIT_OFFSET, PREFAB_BIT_COUNT);
        }



        constexpr decltype(auto) GetOwnerIndex() const {
            return Get(OWNER_BIT_OFFSET, OWNER_BIT_COUNT);
        }

        constexpr void SetOwnerIndex(decltype(m_data) index) {
            Set(index, OWNER_BIT_OFFSET, OWNER_BIT_COUNT);
        }
    } m_encoded;*/

    
    /*
    // Get all machine-representative flags
    constexpr LocalFlag _GetFlags() const {
        return LocalFlag((m_encoded >> FLAG_BIT_COUNT) & FLAG_BIT_MASK);
    }

    // Check whether this ZDO has at least all of the specified flags
    //  Flags may be xored together
    constexpr bool _HasAllFlags(LocalFlag flags) const {
        return (this->_GetFlags() & flags) == flags;
    }

    // Check whether this ZDO has any of the specified flags
    //  Flags may be xored together
    constexpr bool _HasAnyFlags(LocalFlag flags) const {
        return static_cast<bool>(this->_GetFlags() & flags);
    }

    // Check whether this ZDO has any of the specified data types
    constexpr bool _HasDenotion(LocalDenotion denotion) const {
        return _HasAllFlags(LocalFlag(1 << std::to_underlying(denotion)));
    }

    constexpr void SetDenotion(LocalDenotion denotion, bool enable) {
        if (enable) {
            m_encoded |= (static_cast<uint32_t>(1) << denotion) << FLAG_BIT_OFFSET;
        }
        else {
            m_encoded &= ~((static_cast<uint32_t>(1) << denotion) << FLAG_BIT_OFFSET);
        }
    }

    // Check whether this ZDO contains a specific templated member data type
    template<typename T>
        requires is_zdo_member<T>::value
    bool _ContainsMember() const {
        static_assert(
            VUtils::Traits::tuple_index<float, zdo_member_tuple>::value == std::to_underlying(LocalDenotion::Member_Float),
            "zdo_types type indexes must must zdo enum Flags"
            );

        return _HasDenotion(LocalDenotion(VUtils::Traits::tuple_index<T, zdo_member_tuple>::value));
    }    



    constexpr decltype(m_encoded) _GetPrefabIndex() const {
        return (m_encoded >> PREFAB_BIT_OFFSET) & PREFAB_BIT_MASK;
    }

    constexpr decltype(m_encoded) _GetOwnerIndex() const {
        return (m_encoded >> OWNER_BIT_OFFSET) & OWNER_BIT_MASK;
    }

    constexpr void _SetPrefabIndex(decltype(m_encoded) index) {
        m_encoded &= ((index & PREFAB_BIT_MASK) << PREFAB_BIT_OFFSET);
    }

    constexpr void _SetOwnerIndex(decltype(m_encoded) index) {
        m_encoded &= ((index & OWNER_BIT_MASK) << OWNER_BIT_OFFSET);
    }*/

public:
    ZDO();

    // ZDOManager constructor
    ZDO(ZDOID id, Vector3f pos);

    ZDO(const ZDO& other) = default;
    ZDO(ZDO&& other) = default;


    


    // Load ZDO from disk
    //  Returns whether this ZDO is modern
    void Load31Pre(DataReader& reader, int32_t version);

    // Reads from a buffer using the new efficient format (version >= 31)
    //  version=0: Read according to the network deserialize format
    //  version>0: Read according to the file load format
    ZDOConnector::Type Unpack(DataReader& reader, int32_t version);

    // Writes to a buffer using the new efficient format (version >= 31)
    //  If 'network' is true, write according to the network serialize format
    //  Otherwise write according to the file save format
    void Pack(DataWriter& writer, bool network) const;

    // Get a member by hash
    //  Returns null if absent 
    //  Throws on type mismatch
    template<typename T>
        requires is_zdo_member<T>::value
    const T* Get(HASH_t key) const {
        //if (m_encoded.HasMember<T>()) {
        if (m_pack.Get<2>() & (1 << GetMemberDenotion<T>())) {
            auto&& members_find= ZDO_MEMBERS.find(ID());
            if (members_find != ZDO_MEMBERS.end()) {
                auto&& members = members_find->second;

                auto mut = hash_to_xhash<T>(key);

                auto&& find = members.find(mut);
                if (find != members.end()) {
                    return std::get_if<T>(&find->second);
                }
            }
        }
        return nullptr;
    }

    // Get a member by string
    //  Returns null if absent 
    //  Throws on type mismatch
    template<typename T>
        requires is_zdo_member<T>::value
    const T* Get(std::string_view key) const {
        return Get<T>(VUtils::String::GetStableHashCode(key));
    }

    // Trivial hash getters
    template<typename T>
        requires is_zdo_member<T>::value
    const T& Get(HASH_t key, const T& value) const {
        auto&& get = Get<T>(key);
        return get ? *get : value;
    }

    // Hash-key getters
    template<typename T>
        requires is_zdo_member<T>::value
    const T& Get(std::string_view key, const T &value) const { return Get<T>(VUtils::String::GetStableHashCode(key), value); }
        
    float               GetFloat(       HASH_t key, float value) const {                            return Get<float>(key, value); }
    int32_t             GetInt(         HASH_t key, int32_t value) const {                          return Get<int32_t>(key, value); }
    int64_t             GetLong(        HASH_t key, int64_t value) const {                          return Get<int64_t>(key, value); }
    Int64Wrapper        GetLongWrapper( HASH_t key, Int64Wrapper value) const {                     return Get<int64_t>(key, value); }
    Quaternion          GetQuaternion(  HASH_t key, Quaternion value) const {                       return Get<Quaternion>(key, value); }
    Vector3f            GetVector3(     HASH_t key, Vector3f value) const {                         return Get<Vector3f>(key, value); }
    std::string_view    GetString(      HASH_t key, std::string_view value) const {                 auto&& val = Get<std::string>(key); return val ? std::string_view(*val) : value; }
    const BYTES_t*      GetBytes(       HASH_t key) const {                                         return Get<BYTES_t>(key); }
    bool                GetBool(        HASH_t key, bool value) const {                             return GetInt(key, value ? 1 : 0); }
    ZDOID               GetZDOID(const std::pair<HASH_t, HASH_t>& key, ZDOID value) const {         return ZDOID(GetLong(key.first, value.GetOwner()), GetLong(key.second, value.GetUID())); }

    // Hash-key default getters
    float               GetFloat(       HASH_t key) const {                                         return Get<float>(key, {}); }
    int32_t             GetInt(         HASH_t key) const {                                         return Get<int32_t>(key, {}); }
    int64_t             GetLong(        HASH_t key) const {                                         return Get<int64_t>(key, {}); }
    Int64Wrapper        GetLongWrapper( HASH_t key) const {                                         return Get<int64_t>(key, {}); }
    Quaternion          GetQuaternion(  HASH_t key) const {                                         return Get<Quaternion>(key, {}); }
    Vector3f            GetVector3(     HASH_t key) const {                                         return Get<Vector3f>(key, {}); }
    std::string_view    GetString(      HASH_t key) const {                                         return Get<std::string>(key, ""); }
    bool                GetBool(        HASH_t key) const {                                         return Get<int32_t>(key); }
    ZDOID               GetZDOID(       const std::pair<HASH_t, HASH_t>& key) const {               return ZDOID(GetLong(key.first), GetLong(key.second)); }

    // String-key getters
    float               GetFloat(       std::string_view key, float value) const {                  return Get<float>(key, value); }
    int32_t             GetInt(         std::string_view key, int32_t value) const {                return Get<int32_t>(key, value); }
    int64_t             GetLong(        std::string_view key, int64_t value) const {                return Get<int64_t>(key, value); }
    Int64Wrapper        GetLongWrapper( std::string_view key, Int64Wrapper value) const {           return Get<int64_t>(key, value); }
    Quaternion          GetQuaternion(  std::string_view key, Quaternion value) const {             return Get<Quaternion>(key, value); }
    Vector3f            GetVector3(     std::string_view key, Vector3f value) const {               return Get<Vector3f>(key, value); }
    std::string_view    GetString(      std::string_view key, std::string_view value) const {       auto&& val = Get<std::string>(key); return val ? std::string_view(*val) : value; }
    const BYTES_t*      GetBytes(       std::string_view key) const {                               return Get<BYTES_t>(key); }
    bool                GetBool(        std::string_view key, bool value) const {                   return Get<int32_t>(key, value); }
    ZDOID               GetZDOID(       std::string_view key, ZDOID value) const {                  return GetZDOID(ToHashPair(key), value); }

    // String-key default getters
    float               GetFloat(       std::string_view key) const {                               return Get<float>(key, {}); }
    int32_t             GetInt(         std::string_view key) const {                               return Get<int32_t>(key, {}); }
    int64_t             GetLong(        std::string_view key) const {                               return Get<int64_t>(key, {}); }
    Int64Wrapper        GetLongWrapper( std::string_view key) const {                               return Get<int64_t>(key, {}); }
    Quaternion          GetQuaternion(  std::string_view key) const {                               return Get<Quaternion>(key, {}); }
    Vector3f            GetVector3(     std::string_view key) const {                               return Get<Vector3f>(key, {}); }
    std::string_view    GetString(      std::string_view key) const {                               return Get<std::string>(key, ""); }
    bool                GetBool(        std::string_view key) const {                               return Get<int32_t>(key, {}); }
    ZDOID               GetZDOID(       std::string_view key) const {                               return GetZDOID(key, {}); }

    // Trivial hash setters
    template<typename T>
        requires is_zdo_member<T>::value
    void Set(HASH_t key, T value) {
        if (_Set(key, std::move(value)))
            Revise();
    }
    
    // Special hash setters
    void Set(HASH_t key, bool value) { Set(key, value ? (int32_t)1 : 0); }
    void Set(const std::pair<HASH_t, HASH_t>& key, ZDOID value) {
        Set(key.first, value.GetOwner());
        Set(key.second, (int64_t)value.GetUID());
    }

    // Trivial hey-string setters (+bool)

    //template<typename T> requires TrivialSyncType<T> || std::same_as<T, bool>
    //void Set(const std::string& key, const T& value) { Set(VUtils::String::GetStableHashCode(key), value); }
    
    //void Set(const std::string& key, const std::string& value) { Set(VUtils::String::GetStableHashCode(key), value); } // String overload
    // Special string setters

    template<typename T>
        requires is_zdo_member<T>::value
    void Set(std::string_view key, T value) { Set(VUtils::String::GetStableHashCode(key), std::move(value)); }

    void Set(std::string_view key, bool value) { Set(VUtils::String::GetStableHashCode(key), value ? (int32_t)1 : 0); }

    void Set(std::string_view key, ZDOID value) { Set(ToHashPair(key), value); }


    // TODO have compiler settings that include/exclude certain features
    //  ie. ignore Prefab restrictions...
    //  prefab checking...

    ZDOID ID() const {
        return this->m_id;
    }

    Vector3f Position() const {
        return m_pos;
    }

    void SetPosition(Vector3f pos);

    ZoneID GetZone() const;

    Quaternion Rotation() const {
        return Quaternion::Euler(m_rotation.x, m_rotation.y, m_rotation.z);
    }

    void SetRotation(Quaternion rot) {
        auto&& euler = rot.EulerAngles();
        if (euler != m_rotation) {
            m_rotation = euler;
            Revise();
        }
    }
        
    const Prefab& GetPrefab() const {
        //return PrefabManager()->RequirePrefabByIndex(m_encoded.GetPrefabIndex());
        return PrefabManager()->RequirePrefabByIndex(m_pack.Get<1>());
    }
    
    HASH_t GetPrefabHash() const {
        return PrefabManager()->RequirePrefabByIndex(m_pack.Get<1>()).m_hash;
    }

    /*
    void _SetRotation(Quaternion rot) {
        this->m_rotation = rot;
    }*/

    void SetLocalScale(Vector3f scale, bool allowIdentity) {
        // if scale axis' are the same, use scaleScalar
        if (std::abs(scale.x - scale.y) < std::numeric_limits<float>::epsilon() * 8
            && std::abs(scale.y - scale.z) < std::numeric_limits<float>::epsilon() * 8) {

            if (allowIdentity || std::abs(scale.x - 1) > std::numeric_limits<float>::epsilon() * 8) {
                this->Set(Hashes::ZDO::ZNetView::SCALE_SCALAR, scale);
            }
        }
        else {
            // otherwise use scale
            this->Set(Hashes::ZDO::ZNetView::SCALE, scale);
        }
    }

    OWNER_t Owner() const {
        return ZDOID::GetUserIDByIndex(m_pack.Get<0>());
    }



    bool IsOwner(OWNER_t owner) const {
        //return m_encoded.GetOwnerIndex() == owner.
        return owner == this->Owner();
    }

    // Return whether the ZDO instance is self hosted or remotely hosted
    bool IsLocal() const {
        return IsOwner(VH_ID);
    }

    // Whether an owner has been assigned to this ZDO
    bool HasOwner() const {
        return m_pack.Get<0>();
        //return m_encoded.GetOwnerIndex();
        //return _HasDenotion(LocalDenotion::Marker_Owner);
        //return Owner();
    }

    // Claim ownership over this ZDO
    bool SetLocal() {
        return SetOwner(VH_ID);
    }

    // Should name better
    void Disown() {
        SetOwner(0);
    }

    // Set the owner of the ZDO
    //  The owner revision will increase
    bool SetOwner(OWNER_t owner) {
        // only if the owner has changed, then revise it
        if (this->Owner() != owner) {
            _SetOwner(owner);

            m_rev.ReviseOwner();
            return true;
        }
        return false;
    }

    // Set the owner of the ZDO
    //  The owner revision is unaffected
    void _SetOwner(OWNER_t owner) {
        m_pack.Set<0>(ZDOID::EnsureUserIDIndex(owner));
        //m_encoded.SetOwnerIndex(ZDOID::EnsureUserIDIndex(owner));
    }



    uint16_t GetOwnerRevision() const {
        return m_rev.GetOwnerRevision();
    }

    uint32_t GetDataRevision() const {
        return m_rev.GetDataRevision();
    }



    bool IsPersistent() const {
        //return m_encoded.HasDenotion(LocalDenotion::Marker_Persistent);
        return m_pack.Get<2>() & LocalFlag::Marker_Persistent; //.HasDenotion(LocalDenotion::Marker_Persistent);
    }

    bool IsDistant() const {
        return m_pack.Get<2>() & LocalFlag::Marker_Distant;
        //return m_encoded.HasDenotion(LocalDenotion::Marker_Distant);
    }

    Prefab::Type GetType() const {
        return Prefab::Type((m_pack.Get<2>() >> std::to_underlying(LocalDenotion::Marker_Type1)) & 0b11);

        //return m_pack.Get<2>() >>
        //return Prefab::Type((m_encoded.GetFlags() >> std::to_underlying(LocalDenotion::Marker_Type1)) & 0b11);
    }



    size_t GetTotalAlloc() {
        size_t size = 0;
        //for (auto&& pair : m_members) size += sizeof(Ord) + pair.second.GetTotalAlloc();
        return size;
    }
};
