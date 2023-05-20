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

template<typename T>
concept TrivialSyncType = 
       std::same_as<T, float>
    || std::same_as<T, Vector3f>
    || std::same_as<T, Quaternion>
    || std::same_as<T, int32_t>
    || std::same_as<T, int64_t>
    || std::same_as<T, std::string>
    || std::same_as<T, BYTES_t>;

// The 'butter' of Valheim
// This class has been refactored numerous times 
//  Performance is important but memory usage has been highly prioritized here
// This class used to be 500+ bytes
//  It is now 120 bytes 
// This class is finally the smallest it could possibly be (I hope so).
class ZDO {
    friend class IZDOManager;
    friend class IPrefabManager;
    friend class Tests;
    friend class IValhalla;

public:
    struct Rev {
        uint32_t m_dataRev = 0;
        uint32_t m_ownerRev = 0;
        /*
        union {
            // Ticks is used for ZDO creationTime
            TICKS_t m_ticksCreated;

            // Time is used for Peer last ZDO update time
            float m_syncTime;
        };*/
    };

    static std::pair<HASH_t, HASH_t> ToHashPair(std::string_view key) {
        return {
            VUtils::String::GetStableHashCode(std::string(key) + "_u"),
            VUtils::String::GetStableHashCode(std::string(key) + "_i")
        };
    }

private:
    static constexpr HASH_t HASH_TIME_CREATED = __H("__VH_TIME_CREATED__");

    static constexpr uint64_t ENCODED_OWNER_MASK =      0b1000000000000000000000000000000011111111111111111111111111111111ULL;
    static constexpr uint64_t ENCODED_ORDINAL_MASK =    0b0111111100000000000000000000000000000000000000000000000000000000ULL;
    static constexpr uint64_t ENCODED_OWNER_REV_MASK =  0b0000000011111111111111111111111100000000000000000000000000000000ULL;

    using SHIFTHASH_t = uint64_t;

    using Ordinal = uint8_t;

    static constexpr Ordinal ORD_FLOAT = 0;
    static constexpr Ordinal ORD_VECTOR3 = 1;
    static constexpr Ordinal ORD_QUATERNION = 2;
    static constexpr Ordinal ORD_INT = 3;
    static constexpr Ordinal ORD_STRING = 4;
    static constexpr Ordinal ORD_LONG = 6;  
    static constexpr Ordinal ORD_ARRAY = 5; // *IMPORTANT: Valheim member 'mask 'Byte array' mask is 7



    template<TrivialSyncType T>
    static constexpr Ordinal GetOrdinal() {
        if constexpr (std::same_as<T, float>) {
            return ORD_FLOAT;
        }
        else if constexpr (std::same_as<T, Vector3f>) {
            return ORD_VECTOR3;
        }
        else if constexpr (std::same_as<T, Quaternion>) {
            return ORD_QUATERNION;
        }
        else if constexpr (std::same_as<T, int32_t>) {
            return ORD_INT;
        }
        else if constexpr (std::same_as<T, int64_t>) {
            return ORD_LONG;
        }
        else if constexpr (std::same_as<T, std::string>) {
            return ORD_STRING;
        }
        else {
            return ORD_ARRAY;
        }
    }

    template<TrivialSyncType T>
    static constexpr Ordinal GetOrdinalMask() {
        return 0b1 << GetOrdinal<T>();
    }
    
    template<TrivialSyncType T>
    static constexpr SHIFTHASH_t ToShiftHash(HASH_t hash) {
        size_t key = ankerl::unordered_dense::hash<Ordinal>{}(GetOrdinal<T>());
        return static_cast<SHIFTHASH_t>(hash) ^ key;
    }

    template<TrivialSyncType T>
    static constexpr HASH_t FromShiftHash(SHIFTHASH_t hash) {
        size_t key = ankerl::unordered_dense::hash<Ordinal>{}(GetOrdinal<T>());
        return static_cast<HASH_t>(hash ^ key);
    }


private:
    class Ord {
    private:
        std::variant<std::monostate, float, Vector3f, Quaternion, int32_t, int64_t, std::string, BYTES_t> m_data;

    public:
        Ord() {}

        template<TrivialSyncType T>
        Ord(T type) : m_data(std::move(type)) {}

        Ord(const Ord& other) = default;
        Ord(Ord&& other) = default;

        void operator=(const Ord& other) {
            this->m_data = other.m_data;
        }

        bool HasValue() const {
            return static_cast<bool>(std::get_if<std::monostate>(&this->m_data));
        }

        template<TrivialSyncType T>
        bool IsType() const {
            return static_cast<bool>(std::get_if<T>(&this->m_data));
        }

        /*
        // Ensure the underlying type matches
        //  Will throw on type mismatch
        template<TrivialSyncType T>
        void AssertType() const {
            if (!IsType<T>())
                throw std::runtime_error("zdo typemask mismatch");
        }*/

        template<TrivialSyncType T>
        T* Get() {
            auto&& data = std::get_if<T>(&this->m_data);
            if (data) {
                return data;
            }
            throw std::runtime_error("zdo typemask mismatch");
        }

        template<TrivialSyncType T>
        const T* Get() const {
            auto&& data = std::get_if<T>(&this->m_data);
            if (data) {
                return data;
            }
            throw std::runtime_error("zdo typemask mismatch");
        }

        // Reassign the underlying member value
        //  Returns whether the previous value was modified
        //  Will throw on type mismatch
        template<TrivialSyncType T>
        bool Set(T type) {
            auto&& data = Get<T>();

            // if fairly trivial 
            //  not BYTES or string because equality operator for them is O(N)
            if ((!std::is_same_v<T, BYTES_t> && !std::is_same_v<T, std::string>)
                || *data != type) {
                *data = std::move(type);
                return true;
            }

            return false;
        }

        /*
        // Get the underlying member
        //  Will throw on type mismatch
        template<TrivialSyncType T>
        T* Get() {
            AssertType<T>();

            return _Member<T>();
        }

        // Get the underlying member
        //  Will throw on type mismatch
        template<TrivialSyncType T>
        const T* Get() const {
            AssertType<T>();

            return _Member<T>();
        }*/

        // Used when saving or serializing internal ZDO information
        //  Returns whether write was successful (if type match)
        //template<TrivialSyncType T>
        template<TrivialSyncType T>
        bool Write(DataWriter& writer, SHIFTHASH_t shiftHash) const {
            auto&& data = std::get_if<T>(&this->m_data);
            if (data) {
                writer.Write(FromShiftHash<T>(shiftHash));
                if constexpr (std::is_same_v<T, std::string>)
                    writer.Write(std::string_view(*data));
                else
                    writer.Write(*data);
                return true;
            }
            return false;
        }

        size_t GetTotalAlloc() const {
            return std::visit(
                [](const auto& value) -> size_t { if constexpr (VUtils::Traits::is_iterable_v<decltype(value)>) return value.capacity(); else return 0; },
                this->m_data
            );
        }
    };



    // Set the object by hash (Internal use only; does not revise ZDO on changes)
    //  Returns whether the previous value was modified
    //  Throws on type mismatch
    template<TrivialSyncType T>
    bool _Set(HASH_t key, T value) {
        auto mut = ToShiftHash<T>(key);

        // TODO put a null insert up here, then set val or not based on initially present or absent
        //this->m_members[mut] = Ord(value);

        //this->m_encoded |= (static_cast<uint64_t>(GetOrdinalMask<T>()) << (8 * 7));
        //assert(GetOrdinalMask() & GetOrdinalMask<T>());

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
        
        /*
        // Quickly check whether type is in map
        if (GetOrdinalMask() & GetOrdinalMask<T>()) {

            // Check whether the exact hash is in map
            //  If map contains, assumed a value reassignment (of same type)
            auto&& find = m_members.find(mut);
            if (find != m_members.end()) {
                return find->second.Set<T>(value);
            }
        }
        else {
            this->m_encoded |= (static_cast<uint64_t>(GetOrdinalMask<T>()) << (8 * 7));
            assert(GetOrdinalMask() & GetOrdinalMask<T>());
        }
        bool insert = m_members.insert({ mut, Ord(value) }).second;
        assert(insert); // It must be uniquely inserted*/
        return true;
    }

    /*
    bool _Set(HASH_t key, const void* value, Ordinal ordinal) {
        switch (ordinal) {
        case ORD_FLOAT:		    return _Set(key, *(float*)          value);
        case ORD_VECTOR3:		return _Set(key, *(Vector3f*)       value);
        case ORD_QUATERNION:	return _Set(key, *(Quaternion*)     value);
        case ORD_INT:			return _Set(key, *(int32_t*)        value);
        case ORD_LONG:			return _Set(key, *(int64_t*)        value);
        case ORD_STRING:		return _Set(key, *(std::string*)    value);
        case ORD_ARRAY:		    return _Set(key, *(BYTES_t*)        value);
        default:
            // good programming and proper use will prevent this case
            assert(false);
        }
    }*/

    // owner is initially derived by random:
    //  C#  hash code function covers [INT_MIN, INT_MAX],
    //  UnityEngine Range(1, MAX) covers [1, 2^31 - 1)

private:
    void Revise() {
        m_dataRev++;
    }

public:
    uint32_t GetOwnerRevision() const {
        return static_cast<uint32_t>((this->m_encoded >> 32) & 0xFFFFFF);
    }

private:
    void SetOwnerRevision(uint32_t ownerRev) {
        // Zero out owner rev bits
        this->m_encoded &= ~ENCODED_OWNER_REV_MASK;

        // Set owner rev bits
        this->m_encoded |= (static_cast<uint64_t>(ownerRev) << 32) & ENCODED_OWNER_REV_MASK;
    }

    void ReviseOwner() {
        this->m_encoded += 0b0000000000000000000000000000000100000000000000000000000000000000ULL;
    }

    template<typename T, typename CountType>
    void _TryWriteType(DataWriter& writer) const {
        if constexpr (std::is_same_v<CountType, char16_t>)
            writer.Write((BYTE_t)0); // placeholder byte; iffy for c# char (2 bytes .. encoded to max 3)

        if (GetOrdinalMask() & GetOrdinalMask<T>()) {
            // Save structure per each type:
            //  char: count
            //      string: key
            //      F V Q I L S A: value

            if constexpr (!std::is_same_v<CountType, char16_t>)
                writer.Write((BYTE_t)0); // placeholder byte; also 0 byte
                        
            const auto size_mark = writer.Position() - sizeof(BYTE_t);
            CountType count = 0;
            for (auto&& pair : m_members) {
                if (pair.second.Write<T>(writer, pair.first))
                    count++;
                /*
                if constexpr (!std::is_same_v<T, std::string>) {
                    if (pair.second.Write<T>(writer, pair.first))
                        count++;
                }
                else {
                    if (pair.second.Write<std::string_view>(writer, pair.first))
                        count++;
                }*/
            }

            if (count) {
                auto end_mark = writer.Position();
                writer.SetPos(size_mark);

                if constexpr (std::is_same_v<CountType, char16_t>) {
                    auto&& vec = std::get<std::reference_wrapper<BYTES_t>>(writer.m_data).get();
                    auto extraCount = VUtils::String::GetUTF8ByteCount(count) - 1;
                    if (extraCount) {
                        assert(count >= 0x80);
                        // make room for utf8 bytes
                        vec.insert(vec.begin() + size_mark, extraCount, 0);
                        writer.Write((char16_t)count);
                        end_mark += extraCount;
                    } else {
                        assert(count < 0x80);
                        writer.Write((BYTE_t)count); // basic write in place
                    }
                }
                else {
                    writer.Write(count);
                }

                writer.SetPos(end_mark);


            }
        }
    }

    template<typename T, typename CountType>
        requires std::same_as<CountType, char16_t> || std::same_as<CountType, uint8_t>
    void _TryReadType(DataReader& reader) {
        //CountType count = sizeof(CountType) == 2 ? reader.ReadChar() : reader.Read<BYTE_t>();
        decltype(auto) count = reader.Read<CountType>();

        for (int i=0; i < count; i++) {
            // ...fuck
            // https://stackoverflow.com/questions/2934904/order-of-evaluation-in-c-function-parameters
            auto hash(reader.Read<HASH_t>());
            auto type(reader.Read<T>());
            _Set(hash, type);
        }
    }



// 120 bytes:
private:    UNORDERED_MAP_t<SHIFTHASH_t, Ord> m_members;    // 64 bytes (excluding internal alloc)
private:    Quaternion m_rotation;                          // 16 bytes
private:    Vector3f m_pos;                                 // 12 bytes
public:     uint32_t m_dataRev {};                          // 4 bytes (PADDING)
public:     ZDOID m_id;                                     // 8 bytes (encoded)
private:    uint64_t m_encoded {};                          // 8 bytes (encoded<owner, ordinal, ownerRev>)
private:    std::reference_wrapper<const Prefab> m_prefab;  // 8 bytes

       /*
private:    static ankerl::unordered_dense::segmented_map<ZDOID, 
    UNORDERED_MAP_t<SHIFTHASH_t, 
        std::variant<OWNER_t char*
        */



private:
    Ordinal GetOrdinalMask() const {
        return static_cast<Ordinal>((this->m_encoded >> (8 * 7)) & 0b01111111);
    }

    void SetOrdinalMask(Ordinal ord) {
        //assert(std::make_signed_t<decltype(ord)>(ord) >= 0);
        assert((ord & 0b10000000) == 0);

        // Set ORDINAL bits to 0
        this->m_encoded &= ~ENCODED_ORDINAL_MASK;

        assert(GetOrdinalMask() == 0);

        // Set ordinal bits accordingly
        this->m_encoded |= (static_cast<uint64_t>(ord) << (8 * 7));

        assert(GetOrdinalMask() == ord);
    }

    // Set the owner without revising
    void _SetOwner(OWNER_t owner) {
        if (!(owner >= -2147483647LL && owner <= 4294967293LL)) {
            // Ensure filler complement bits are all the same (full negative or full positive)
            //if ((owner < 0 && (static_cast<uint64_t>(owner) & ~ENCODED_OWNER_MASK) != ~ENCODED_OWNER_MASK)
                //|| (owner >= 0 && (static_cast<uint64_t>(owner) & ~ENCODED_OWNER_MASK) == ~ENCODED_OWNER_MASK))
            throw std::runtime_error("OWNER_t unexpected encoding (client Utils.GenerateUID() differs?)");
        }

        // Zero out the owner bytes (including sign)
        this->m_encoded &= ~ENCODED_OWNER_MASK;
        assert(Owner() == 0);

        // Set the owner bytes
        //  ignore the 2's complement middle bytes
        this->m_encoded |= (static_cast<uint64_t>(owner) & ENCODED_OWNER_MASK);

        assert(Owner() == owner);
    }

public:
    ZDO();

    // ZDOManager constructor
    ZDO(ZDOID id, Vector3f pos);

    //ZDO(const ZDOID& id, const Vector3f& pos, HASH_t prefab);

    //ZDO(const ZDOID& id, const Vector3f& pos, DataReader& deserialize, uint32_t ownerRev, uint32_t dataRev);

    ZDO(const ZDO& other) = default;
    


    // Save ZDO to disk
    void Save(DataWriter& writer) const;

    // Load ZDO from disk
    //  Returns whether this ZDO is modern
    bool Load31Pre(DataReader& reader, int32_t version);

    bool LoadPost31(DataReader& reader, int32_t version);



    // Get a member by hash
    //  Returns null if absent 
    //  Throws on type mismatch
    template<TrivialSyncType T>
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
    template<TrivialSyncType T>
    const T* Get(std::string_view key) const {
        return Get<T>(VUtils::String::GetStableHashCode(key));
    }

    // Trivial hash getters
    template<TrivialSyncType T>
    const T& Get(HASH_t key, const T& value) const {
        auto&& get = Get<T>(key);
        return get ? *get : value;
    }

    // Hash-key getters
    template<TrivialSyncType T>
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
    template<TrivialSyncType T>
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

    template<TrivialSyncType T>
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
        return m_rotation;
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
        // If owner is negative (sign bit)
        //  Then return a 1-bit unused middle portion of bits to maintain negative number)
        if (std::make_signed_t<decltype(m_encoded)>(m_encoded) < 0)
            return static_cast<OWNER_t>((m_encoded & ENCODED_OWNER_MASK) | ~ENCODED_OWNER_MASK);
        return static_cast<OWNER_t>(m_encoded & ENCODED_OWNER_MASK);
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
        return !IsOwner(0);
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
            this->_SetOwner(owner);

            ReviseOwner();
            return true;
        }
        return false;
    }

    TICKS_t GetTimeCreated() const {
        if (GetPrefab().AnyFlagsPresent(Prefab::Flag::TERRAIN_MODIFIER | Prefab::Flag::DUNGEON))
            return TICKS_t(GetLong(HASH_TIME_CREATED));
        return {};
    }

    void SetTimeCreated(TICKS_t ticks) {
        if (GetPrefab().AnyFlagsPresent(Prefab::Flag::TERRAIN_MODIFIER | Prefab::Flag::DUNGEON))
            Set(HASH_TIME_CREATED, ticks.count());
    }



    size_t GetTotalAlloc() {
        size_t size = 0;
        for (auto&& pair : m_members) size += sizeof(Ord) + pair.second.GetTotalAlloc();
        return size;
    }

    // Save ZDO to network packet
    void Serialize(DataWriter& pkg) const;

    // Load ZDO from network packet
    void Deserialize(DataReader& pkg);
};
