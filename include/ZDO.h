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

class IZDOManager;

// 500+ bytes (7 maps)
// 168 bytes (1 map)
// 112 bytes (1 map, majorly reduced members; affecting functionality)
// Currently 160 bytes
class ZDO {
    friend class IZDOManager;
    friend class IPrefabManager;
    friend class Tests;

public:
    struct Rev {
        uint32_t m_dataRev = 0;
        uint32_t m_ownerRev = 0;

        union {
            // Ticks is used for ZDO creationTime
            TICKS_t m_ticksCreated;

            // Time is used for Peer last ZDO update time
            float m_syncTime;
        };
    };

    static std::pair<HASH_t, HASH_t> ToHashPair(const std::string& key);

private:
    using SHIFTHASH_t = uint64_t;

    using Ordinal = uint8_t;

    static constexpr Ordinal ORD_FLOAT = 0;
    static constexpr Ordinal ORD_VECTOR3 = 1;
    static constexpr Ordinal ORD_QUATERNION = 2;
    static constexpr Ordinal ORD_INT = 3;
    static constexpr Ordinal ORD_STRING = 4;
    static constexpr Ordinal ORD_LONG = 6;    
    static constexpr Ordinal ORD_ARRAY = 7;



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
        // Allocated bytes of [Ordinal, member...]
        //  First byte is Ordinal
        //  Remaining allocation is type
        
        BYTE_t* m_contiguous;

        static_assert(sizeof(*m_contiguous) == 1);

        Ordinal* _Ordinal() {
            return (Ordinal*)m_contiguous;
        }

        const Ordinal* _Ordinal() const {
            return (Ordinal*)m_contiguous;
        }

        template<TrivialSyncType T>
        T* _Member() {
            return (T*)(m_contiguous + sizeof(Ordinal));
        }

        template<TrivialSyncType T>
        const T* _Member() const {
            return (T*)(m_contiguous + sizeof(Ordinal));
        }

        template<TrivialSyncType T>
        void _ForceReAssign(const T &type) {
            if (HasValue()) {
                if (*_Ordinal() != GetOrdinal<T>()) {
                    // call destructor of self
                    switch (*_Ordinal()) {
                    case ORD_STRING: _Member<std::string>()->~basic_string(); break;
                    case ORD_ARRAY: _Member<BYTES_t>()->~vector(); break;
                    default:
                        assert(false);
                    }

                    // try resizing memory

                    if (auto mem = (BYTE_t*) std::realloc(this->m_contiguous, sizeof(Ordinal) + sizeof(T)))
                        this->m_contiguous = mem;
                    else
                        throw std::runtime_error("failed to realloc zdo");
                }
            } 
            else {// allocate a new block
                if (!(this->m_contiguous = (BYTE_t*)std::malloc(sizeof(Ordinal) + sizeof(T))))
                    throw std::runtime_error("failed to malloc zdo");
            }

            *this->_Ordinal() = GetOrdinal<T>();

            // https://stackoverflow.com/questions/2494471/c-is-it-possible-to-call-a-constructor-directly-without-new
            new (this->_Member<T>()) T(type);
        }

    public:
        Ord() : m_contiguous(nullptr) {}

        template<TrivialSyncType T>
        Ord(const T &type) : m_contiguous(nullptr) {
            _ForceReAssign(type);
        }

        Ord(const Ord& other) : m_contiguous(nullptr) {
            *this = other;
        }

        Ord(Ord&& other) noexcept {
            this->m_contiguous = other.m_contiguous;
            other.m_contiguous = nullptr;
        }

        ~Ord() {
            if (HasValue()) {
                switch (*_Ordinal()) {
                case ORD_FLOAT: break;
                case ORD_VECTOR3: break;
                case ORD_QUATERNION: break;
                case ORD_INT: break;
                case ORD_LONG: break;
                case ORD_STRING: _Member<std::string>()->~basic_string(); break;
                case ORD_ARRAY: _Member<BYTES_t>()->~vector(); break;
                default:
                    assert(false && "reached impossible case");
                }

                free(m_contiguous);
            }
        }

        void operator=(const Ord& other) {
            if (HasValue()) {
                const auto ord = *other._Ordinal();
                switch (ord) {
                case ORD_FLOAT:         _ForceReAssign(*other._Member<float>()); break;
                case ORD_VECTOR3:       _ForceReAssign(*other._Member<Vector3f>()); break;
                case ORD_QUATERNION:    _ForceReAssign(*other._Member<Quaternion>()); break;
                case ORD_INT:           _ForceReAssign(*other._Member<int32_t>()); break;
                case ORD_LONG:          _ForceReAssign(*other._Member<int64_t>()); break;
                case ORD_STRING:        _ForceReAssign(*other._Member<std::string>()); break;
                case ORD_ARRAY:         _ForceReAssign(*other._Member<BYTES_t>()); break;
                default:
                    assert(false && "reached impossible case");
                }
            }
            else {
                this->~Ord();
                this->m_contiguous = nullptr;
            }
        }

        bool HasValue() const {
            return static_cast<bool>(this->m_contiguous);
        }

        template<TrivialSyncType T>
        bool IsType() const {
            return HasValue() && * _Ordinal() == GetOrdinal<T>();
        }

        // Ensure the underlying type matches
        //  Will throw on type mismatch
        template<TrivialSyncType T>
        void AssertType() const {
            if (!IsType<T>())
                throw std::runtime_error("zdo typemask mismatch");
        }

        // Reassign the underlying member value
        //  Returns whether the previous value was modified
        //  Will throw on type mismatch
        template<TrivialSyncType T>
        bool Set(const T& type) {
            AssertType<T>();

            // if fairly trivial 
            //  not BYTES or string because equality operator for them is O(N)
            if ((!std::is_same_v<T, BYTES_t> && !std::is_same_v<T, std::string>)
                || *_Member<T>() != type) {
                *_Member<T>() = type;
                return true;
            }

            return false;
        }

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
        }

        // Used when saving or serializing internal ZDO information
        //  Returns whether write was successful (if type match)
        template<TrivialSyncType T>
        bool Write(DataWriter& writer, SHIFTHASH_t shiftHash) const {
            if (!IsType<T>())
                return false;

            writer.Write(FromShiftHash<T>(shiftHash));
            writer.Write(*_Member<T>());
            return true;
        }

        size_t GetTotalAlloc() {
            switch (*_Ordinal()) {
            case ORD_FLOAT:         return sizeof(Ordinal) + sizeof(float);
            case ORD_VECTOR3:       return sizeof(Ordinal) + sizeof(Vector3f);
            case ORD_QUATERNION:    return sizeof(Ordinal) + sizeof(Quaternion);
            case ORD_INT:           return sizeof(Ordinal) + sizeof(int32_t);
            case ORD_LONG:          return sizeof(Ordinal) + sizeof(int64_t);
            case ORD_STRING:        return sizeof(Ordinal) + sizeof(std::string) + _Member<std::string>()->capacity();
            case ORD_ARRAY:         return sizeof(Ordinal) + sizeof(BYTES_t) + _Member<BYTES_t>()->capacity();
            default:
                assert(false && "reached impossible case");
            }
            return 0;
        }
    };



    // Set the object by hash (Internal use only; does not revise ZDO on changes)
    //  Returns whether the previous value was modified
    //  Throws on type mismatch
    template<TrivialSyncType T>
    bool _Set(HASH_t key, const T& value) {
        auto mut = ToShiftHash<T>(key);

        // Quickly check whether type is in map
        if (m_ordinalMask & GetOrdinalMask<T>()) {

            // Check whether the exact hash is in map
            //  If map contains, assumed a value reassignment (of same type)
            auto&& find = m_members.find(mut);
            if (find != m_members.end()) {
                return find->second.Set<T>(value);
            }
        }
        else {
            m_ordinalMask |= GetOrdinalMask<T>();
        }
        bool insert = m_members.insert({ mut, Ord(value) }).second;
        assert(insert); // It must be uniquely inserted
        return true;
    }

    bool _Set(HASH_t key, const void* value, Ordinal ordinal) {
        switch (ordinal) {
        case ORD_FLOAT:		    return _Set(key, *(float*)         value);
        case ORD_VECTOR3:		return _Set(key, *(Vector3f*)       value);
        case ORD_QUATERNION:	return _Set(key, *(Quaternion*)    value);
        case ORD_INT:			return _Set(key, *(int32_t*)       value);
        case ORD_LONG:			return _Set(key, *(int64_t*)       value);
        case ORD_STRING:		return _Set(key, *(std::string*)   value);
        case ORD_ARRAY:		    return _Set(key, *(BYTES_t*)       value);
        default:
            // good programming and proper use will prevent this case
            assert(false);
        }
    }

    // owner is initially derived by random:
    //  C#  hash code function covers [INT_MIN, INT_MAX],
    //  UnityEngine Range(1, MAX) covers [1, 2^31 - 1)

private:
    void Revise() {
        m_rev.m_dataRev++;
    }

    template<typename T, typename CountType>
    void _TryWriteType(DataWriter& writer) const {
        if constexpr (sizeof(CountType) == 2)
            writer.Write((BYTE_t)0); // placeholder byte; iffy for c# char (2 bytes .. encoded to max 3)

        if (m_ordinalMask & GetOrdinalMask<T>()) {
            // Save structure per each type:
            //  char: count
            //      string: key
            //      F V Q I L S A: value

            if constexpr (sizeof(CountType) == 1)
                writer.Write((BYTE_t)0); // placeholder byte; also 0 byte
                        
            const auto size_mark = writer.Position() - sizeof(BYTE_t);
            CountType count = 0;
            for (auto&& pair : m_members) {
                if (pair.second.Write<T>(writer, pair.first))
                    count++;
            }

            if (count) {
                auto end_mark = writer.Position();
                writer.SetPos(size_mark);

                if constexpr (sizeof(CountType) == 2) {
                    auto&& vec = writer.m_provider.get();
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
        requires std::same_as<CountType, char16_t> || std::same_as<CountType, BYTE_t>
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
    /*
// 136 bytes
private:    Quaternion m_rotation;
private:    uint8_t m_ownerIdx;
private:    uint8_t m_uuidIdx;
private:    uint8_t m_ordinalMask;
private:    uint32_t m_id;
//private:    OWNER_t m_owner = 0;
//private:    int64_t m_
private:    std::reference_wrapper<const Prefab> m_prefab;
private:    robin_hood::unordered_map<SHIFTHASH_t, Ord> m_members;
//private:    Ordinal m_ordinalMask = 0;
private:    Vector3f m_pos;

public:     Rev m_rev = {}; // TODO use smaller type for rev and timeCreated
//private:     ZDOID m_id;
//private: OWNER_t m_uuid = 0;
//private: uint32_t m_id = 0;
*/



/*
// 152 bytes:
private:    Quaternion m_rotation;
private:    OWNER_t m_owner = 0;
private:    std::reference_wrapper<const Prefab> m_prefab;
private:    robin_hood::unordered_map<SHIFTHASH_t, Ord> m_members;
private:    Ordinal m_ordinalMask = 0;
private:    Vector3f m_pos;

public:     Rev m_rev = {}; // TODO use smaller type for rev and timeCreated
private:     ZDOID m_id;
      //private: OWNER_t m_uuid = 0;
      //private: uint32_t m_id = 0;
*/




// 160 bytes:
private:    Quaternion m_rotation;
private:    OWNER_t m_owner = 0;
private:    std::reference_wrapper<const Prefab> m_prefab;
private:    UNORDERED_MAP_t<SHIFTHASH_t, Ord> m_members;
private:    Vector3f m_pos;

public:     Rev m_rev = {}; // TODO use smaller type for rev and timeCreated
private:    ZDOID m_id;
private:    Ordinal m_ordinalMask = 0;




public:
    ZDO();

    // ZDOManager constructor
    ZDO(const ZDOID& id, const Vector3f& pos);

    //ZDO(const ZDOID& id, const Vector3f& pos, HASH_t prefab);

    //ZDO(const ZDOID& id, const Vector3f& pos, DataReader& deserialize, uint32_t ownerRev, uint32_t dataRev);

    ZDO(const ZDO& other) = default;
    


    // Save ZDO to disk
    void Save(DataWriter& writer) const;

    // Load ZDO from disk
    //  Returns whether this ZDO is modern
    bool Load(DataReader& reader, int32_t version);



    // Get a member by hash
    //  Returns null if absent 
    //  Throws on type mismatch
    template<TrivialSyncType T>
    const T* Get(HASH_t key) const {
        if (m_ordinalMask & GetOrdinalMask<T>()) {
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
    const T* Get(const std::string& key) const {
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
    const T& Get(const std::string& key, const T& value) const { return Get<T>(VUtils::String::GetStableHashCode(key), value); }
        
    float               GetFloat(       HASH_t key, float value) const {                            return Get<float>(key, value); }
    int32_t             GetInt(         HASH_t key, int32_t value) const {                          return Get<int32_t>(key, value); }
    int64_t             GetLong(        HASH_t key, int64_t value) const {                          return Get<int64_t>(key, value); }
    Int64Wrapper        GetLongWrapper( HASH_t key, const Int64Wrapper& value) const {              return Get<int64_t>(key, value); }
    Quaternion          GetQuaternion(  HASH_t key, const Quaternion& value) const {                return Get<Quaternion>(key, value); }
    Vector3f             GetVector3(     HASH_t key, const Vector3f& value) const {                   return Get<Vector3f>(key, value); }
    std::string         GetString(      HASH_t key, const std::string& value) const {               return Get<std::string>(key, value); }
    const BYTES_t*      GetBytes(       HASH_t key) const {                                         return Get<BYTES_t>(key); }
    bool                GetBool(        HASH_t key, bool value) const {                             return GetInt(key, value ? 1 : 0); }
    ZDOID               GetZDOID(const std::pair<HASH_t, HASH_t>& key, const ZDOID& value) const {  return ZDOID(GetLong(key.first, value.m_uuid), GetLong(key.second, value.m_id)); }

    // Hash-key default getters
    float               GetFloat(       HASH_t key) const {                                         return Get<float>(key, {}); }
    int32_t             GetInt(         HASH_t key) const {                                         return Get<int32_t>(key, {}); }
    int64_t             GetLong(        HASH_t key) const {                                         return Get<int64_t>(key, {}); }
    Int64Wrapper        GetLongWrapper( HASH_t key) const {                                         return Get<int64_t>(key, {}); }
    Quaternion          GetQuaternion(  HASH_t key) const {                                         return Get<Quaternion>(key, {}); }
    Vector3f             GetVector3(     HASH_t key) const {                                         return Get<Vector3f>(key, {}); }
    std::string         GetString(      HASH_t key) const {                                         return Get<std::string>(key, {}); }
    bool                GetBool(        HASH_t key) const {                                         return Get<int32_t>(key); }
    ZDOID               GetZDOID(       const std::pair<HASH_t, HASH_t>& key) const {               return ZDOID(GetLong(key.first), GetLong(key.second)); }

    // String-key getters
    float               GetFloat(       const std::string& key, float value) const {                return Get<float>(key, value); }
    int32_t             GetInt(         const std::string& key, int32_t value) const {              return Get<int32_t>(key, value); }
    int64_t             GetLong(        const std::string& key, int64_t value) const {              return Get<int64_t>(key, value); }
    Int64Wrapper        GetLongWrapper( const std::string& key, const Int64Wrapper& value) const {  return Get<int64_t>(key, value); }
    Quaternion          GetQuaternion(  const std::string& key, const Quaternion& value) const {    return Get<Quaternion>(key, value); }
    Vector3f             GetVector3(     const std::string& key, const Vector3f& value) const {       return Get<Vector3f>(key, value); }
    std::string         GetString(      const std::string& key, const std::string& value) const {   return Get<std::string>(key, value); }
    const BYTES_t*      GetBytes(       const std::string& key) const {                             return Get<BYTES_t>(key); }
    bool                GetBool(        const std::string& key, bool value) const {                 return Get<int32_t>(key, value); }
    ZDOID               GetZDOID(       const std::string& key, const ZDOID& value) const {         return GetZDOID(ToHashPair(key), value); }

    // String-key default getters
    float               GetFloat(const std::string& key) const {                                    return Get<float>(key, {}); }
    int32_t             GetInt(const std::string& key) const {                                      return Get<int32_t>(key, {}); }
    int64_t             GetLong(const std::string& key) const {                                     return Get<int64_t>(key, {}); }
    Int64Wrapper        GetLongWrapper(const std::string& key) const {                              return Get<int64_t>(key, {}); }
    Quaternion          GetQuaternion(const std::string& key) const {                               return Get<Quaternion>(key, {}); }
    Vector3f             GetVector3(const std::string& key) const {                                  return Get<Vector3f>(key, {}); }
    std::string         GetString(const std::string& key) const {                                   return Get<std::string>(key, {}); }
    bool                GetBool(const std::string& key) const {                                     return Get<int32_t>(key, {}); }
    ZDOID               GetZDOID(const std::string& key) const {                                    return GetZDOID(key, {}); }



    // Trivial hash setters
    template<TrivialSyncType T>
    void Set(HASH_t key, const T& value) {
        if (_Set(key, value))
            Revise();
    }
    
    // Special hash setters
    void Set(HASH_t key, bool value) { Set(key, value ? (int32_t)1 : 0); }
    void Set(const std::pair<HASH_t, HASH_t>& key, const ZDOID& value) {
        Set(key.first, value.m_uuid);
        Set(key.second, (int64_t)value.m_id);
    }

    // Trivial hey-string setters (+bool)

    //template<typename T> requires TrivialSyncType<T> || std::same_as<T, bool>
    //void Set(const std::string& key, const T& value) { Set(VUtils::String::GetStableHashCode(key), value); }
    
    //void Set(const std::string& key, const std::string& value) { Set(VUtils::String::GetStableHashCode(key), value); } // String overload
    // Special string setters

    template<TrivialSyncType T>
    void Set(const std::string &key, const T& value) { Set(VUtils::String::GetStableHashCode(key), value); }

    void Set(const std::string& key, bool value) { Set(VUtils::String::GetStableHashCode(key), value ? (int32_t)1 : 0); }

    void Set(const std::string& key, const ZDOID& value) { Set(ToHashPair(key), value); }


    // TODO have compiler settings that include/exclude certain features
    //  ie. ignore Prefab restrictions...
    //  prefab checking...

    ZDOID ID() const {
        return m_id;
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
        return m_owner;
    }

    bool IsOwner(OWNER_t owner) const {
        return this->m_owner == owner;
    }

    // Return whether the ZDO instance is self hosted or remotely hosted
    bool IsLocal() const {
        return IsOwner(SERVER_ID);
    }

    // Whether an owner has been assigned to this ZDO
    bool HasOwner() const {
        return IsOwner(0);
    }

    // Claim ownership over this ZDO
    bool SetLocal() {
        return SetOwner(SERVER_ID);
    }

    // Should name better
    void Disown() {
        SetOwner(0);
    }

    // set the owner of the ZDO
    bool SetOwner(OWNER_t owner) {
        // only if the owner has changed, then revise it
        if (this->m_owner != owner) {
            this->m_owner = owner;
            this->m_rev.m_ownerRev++;
            return true;
        }
        return false;
    }

    bool Valid() const {
        if (m_id)
            return true;
        return false;
    }



    size_t GetTotalAlloc() {
        size_t size = 0;
        for (auto&& pair : m_members) size += pair.second.GetTotalAlloc();
        return size;
    }

    // Save ZDO to network packet
    void Serialize(DataWriter& pkg) const;

    // Load ZDO from network packet
    void Deserialize(DataReader& pkg);
};
