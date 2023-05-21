#pragma once

#include <type_traits>
#include <algorithm>

#include "VUtils.h"
#include "VUtilsString.h"
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

    using zdo_mask = uint16_t;
    using zdo_xhash = uint64_t;

    using zdo_member_tuple = std::tuple<float, Vector3f, Quaternion, int32_t, int64_t, std::string, BYTES_t>;
    using zdo_member_variant = VUtils::Traits::tuple_to_variant<zdo_member_tuple>::type;

    using zdo_member_map = UNORDERED_MAP_t<zdo_xhash, zdo_member_variant>;

    template<typename T>
    struct is_zdo_member : VUtils::Traits::tuple_has_type<T, zdo_member_tuple> {};

public:
    /*
    struct Rev {
        uint32_t m_dataRev = 0;
        uint32_t m_ownerRev = 0;
        
        union {
            // Ticks is used for ZDO creationTime
            TICKS_t m_ticksCreated;

            // Time is used for Peer last ZDO update time
            float m_syncTime;
        };
    };*/

    static std::pair<HASH_t, HASH_t> ToHashPair(std::string_view key) {
        return {
            VUtils::String::GetStableHashCode(std::string(key) + "_u"),
            VUtils::String::GetStableHashCode(std::string(key) + "_i")
        };
    }

private:
    enum class Data : uint16_t {
        //Type_None,
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
        Marker_Owner,
        //Marker_Age,
    };

    enum class Flag : zdo_mask {
        //Type_None,
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
        Marker_Owner = 1 << 13,
        //Marker_Age = 1 << 14,
    };

private:

    // Set the object by hash (Internal use only; does not revise ZDO on changes)
    //  Returns whether the previous value was modified
    //  Throws on type mismatch
    template<typename T> 
        requires is_zdo_member<T>::value
    bool _Set(HASH_t key, T value) {
        auto mut = hash_to_xhash<T>(key);

        auto &&insert = this->m_members.insert({ mut, Ord() });
        if (insert.second) {
            // Manually initial-set
            insert.first->second = Ord(std::move(value));
            this->m_encoded |= (static_cast<uint64_t>(GetOrdinalMask<T>()) << (8 * 7));
            assert(GetOrdinalMask() & GetOrdinalMask<T>());
            return true;
        }
        else {
            assert(GetOrdinalMask() & GetOrdinalMask<T>());
            return insert.first->second.Set<T>(std::move(value));
        }
        
        return true;
    }

private:
    void Revise() {
        m_dataRev++;
    }

    template<typename T>
        requires is_zdo_member<T>::value
    zdo_xhash hash_to_xhash(HASH_t in) const {
        return static_cast<zdo_xhash>(in) 
            ^ static_cast<zdo_xhash>(ankerl::unordered_dense::hash<T>{}(VUtils::Traits::tuple_index<typename T, zdo_member_tuple>::value));
    }

    template<typename T>
        requires is_zdo_member<T>::value
    HASH_t xhash_to_hash(zdo_xhash in) const {
        return static_cast<HASH_t>(in) 
            ^ static_cast<HASH_t>(ankerl::unordered_dense::hash<T>{}(VUtils::Traits::tuple_index<typename T, zdo_member_tuple>::value));
    }

public:
    decltype(auto) GetOwnerRevision() const {
        return this->m_ownerRev;
    }

    void SetOwnerRevision(uint16_t rev) {
        this->m_ownerRev = rev;
    }

    void SetDataRevision(uint32_t rev) {
        this->m_dataRev = rev;
    }

private:
    // Check whether this ZDO has at least all the specified flags
    bool _HasAllFlags(Flag flag) const {
        return (this->m_flags & (flag)) == std::to_underlying(flag);
    }

    // Check whether this ZDO has any of the specified flags
    bool _HasAnyFlags(Flag flags) const {
        return this->m_flags & flags;
    }

    // Check whether this ZDO has any of the specified data types
    bool _HasData(Data data) const {
        return _HasAllFlags(Flag(1 << std::to_underlying(data)));
    }

    // Check whether this ZDO contains a specific templated member data type
    template<typename T>
        requires is_zdo_member<T>::value
    bool _ContainsMember() const {
        static_assert(
            VUtils::Traits::tuple_index<float, zdo_member_tuple>::value + 1 == std::to_underlying(Data::Member_Float),
            "zdo_types type indexes must must zdo enum Flags"
        );

        return _HasData(Data(VUtils::Traits::tuple_index<T, zdo_member_tuple>::value + 1));
    }

    template<typename T>
        requires is_zdo_member<T>::value
    void _TryWriteType(DataWriter& writer, zdo_member_map& members) const {
        if (_ContainsMember<T>()) {     
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
    void _TryReadType(DataReader& reader) {
        decltype(auto) count = reader.Read<CountType>();

        for (int i=0; i < count; i++) {
            // ...fuck
            // https://stackoverflow.com/questions/2934904/order-of-evaluation-in-c-function-parameters
            auto hash(reader.Read<HASH_t>());
            auto type(reader.Read<T>());
            _Set(hash, type);
        }
    }



// 48 bytes:
private:    Vector3f m_pos;                                 // 12 bytes
public:     ZDOID m_id;                                     // 4 bytes (PADDING)
private:    Vector3f m_rotation;                            // 12 bytes
public:     uint32_t m_dataRev{};                           // 4 bytes (PADDING)
private:    std::reference_wrapper<const Prefab> m_prefab;  // 8 bytes
private:    uint16_t m_ownerRev{};                          // 2 bytes
private:    zdo_mask m_flags{};                               // 2 bytes
// padding here                                             // 4 bytes (PADDING)
// TODO do something with this padding
// 16M is the extreme upper bound on data revision
// IDEAS:
//  use the final 4 bytes to store owner index (instead of in a COMPLETELY SEPARATE MAP)
//      I dont think thid 4-byte padding problem exists on esp32 (the ref<prefab> would be 4 bytes, and would have no padding)

private:    
    static ankerl::unordered_dense::segmented_map<ZDOID, zdo_member_map> ZDO_TYPES;
    static ankerl::unordered_dense::segmented_map<ZDOID, ZDOConnector> ZDO_CONNECTORS;
    static ankerl::unordered_dense::segmented_map<ZDOID, OWNER_t> ZDO_OWNERS; // TODO owners could use a similar zdoid indexing procedure (instead of whole uint64 when specific ids are used)
    //static ankerl::unordered_dense::segmented_map<ZDOID, TICKS_t> ZDO_AGES;

public:
    ZDO();

    // ZDOManager constructor
    ZDO(ZDOID id, Vector3f pos);

    ZDO(const ZDO& other) = default;
    


    // Save ZDO to disk
    void Save(DataWriter& writer) const;

    // Load ZDO from disk
    //  Returns whether this ZDO is modern
    void Load31Pre(DataReader& reader, int32_t version);

    //bool LoadPost31(DataReader& reader, int32_t version);

    // Initializes the ZDO using the new efficient format (version >= 31)
    //  version=0: Unpack zdo as a packet
    //  version>0: Unpack zdo as a file
    ZDOConnector::Type Unpack(DataReader& reader, int32_t version);



    // Get a member by hash
    //  Returns null if absent 
    //  Throws on type mismatch
    template<typename T>
        requires is_zdo_member<T>::value
    const T* Get(HASH_t key) const {
        if (GetOrdinalMask() & GetOrdinalMask<T>()) {
            auto mut = ToShiftHash<T>(key);
            auto&& find = m_members.find(mut);
            if (find != m_members.end()) {
                return find->second.Get<T>();
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

    void SetPosition(const Vector3f& pos);

    ZoneID GetZone() const;

    const Quaternion& Rotation() const {
        return Quaternion::Euler(m_rotation.x, m_rotation.y, m_rotation.z);
    }

    void SetRotation(const Quaternion& rot) {

        if (rot != m_rotation) {
            m_rotation = rot;
            Revise();
        }
    }

    const Prefab& GetPrefab() const {
        return m_prefab;
    }

    OWNER_t Owner() const {
        if (_HasData(Data::Marker_Owner)) {
            auto&& find = ZDO_OWNERS.find(ID());
            if (find != ZDO_OWNERS.end()) {
                return find->second;
            }
            assert(false);
        }
        return 0;
    }

    bool IsOwner(OWNER_t owner) const {
        return owner == this->Owner();
    }

    // Return whether the ZDO instance is self hosted or remotely hosted
    bool IsLocal() const {
        return IsOwner(VH_ID);
    }

    // Whether an owner has been assigned to this ZDO
    bool HasOwner() const {
        return _HasData(Data::Marker_Owner);
    }

    // Claim ownership over this ZDO
    bool SetLocal() {
        return SetOwner(VH_ID);
    }

    // Should name better
    void Disown() {
        SetOwner(0);
    }

    // set the owner of the ZDO
    bool SetOwner(OWNER_t owner) {
        // only if the owner has changed, then revise it
        if (this->Owner() != owner) {
            //this->_SetOwner(owner);

            ZDO_OWNERS[ID()] = owner;

            ReviseOwner();
            return true;
        }
        return false;
    }

    TICKS_t GetTimeCreated() const {
        if (_HasData(Data::Marker_Age)) {
            auto&& find = ZDO_AGES.find(ID());
            if (find != ZDO_AGES.end()) {
                return find->second;
            }
            assert(false);
        }
        return {};
    }

    void SetTimeCreated(TICKS_t ticks) {
        ZDO_AGES[ID()] = ticks;
    }



    size_t GetTotalAlloc() {
        size_t size = 0;
        //for (auto&& pair : m_members) size += sizeof(Ord) + pair.second.GetTotalAlloc();
        return size;
    }

    // Save ZDO to network packet
    void Serialize(DataWriter& pkg) const;

    // Load ZDO from network packet
    void Deserialize(DataReader& pkg);
};
