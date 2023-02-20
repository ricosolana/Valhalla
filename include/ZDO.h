#pragma once

#include <robin_hood.h>
#include <type_traits>
#include <algorithm>

#include "HashUtils.h"
#include "Quaternion.h"
#include "Vector.h"
#include "VUtils.h"
#include "VUtilsString.h"
#include "DataWriter.h"
#include "DataReader.h"

template<typename T>
concept TrivialSyncType = 
       std::same_as<T, float>
    || std::same_as<T, Vector3>
    || std::same_as<T, Quaternion>
    || std::same_as<T, int32_t>
    || std::same_as<T, int64_t>
    || std::same_as<T, std::string>
    || std::same_as<T, BYTES_t>;

class IZDOManager;
class IPrefabManager;
class Prefab;

// 500+ bytes (7 maps)
// 168 bytes (1 map)
// 112 bytes (1 map, majorly reduced members; affecting functionality)
// Currently 160 bytes
class ZDO {
    friend class IZDOManager;
    friend class IPrefabManager;

public:
    enum class ObjectType : BYTE_t {
        Default,
        Prioritized,
        Solid,
        Terrain
    };

    struct Rev {
        uint32_t m_dataRev = 0;
        uint32_t m_ownerRev = 0;

        union {
            TICKS_t m_ticks;
            float m_time;
        };
    };

    template<TrivialSyncType T>
    class ProxyMember {
        friend class ZDO;

    private:
        ZDO& m_zdo; // The ZDO to which this member belongs
        T* m_member;

    private:
        ProxyMember(ZDO& zdo) : m_zdo(zdo), m_member(nullptr) {}
        ProxyMember(ZDO& zdo, T& member) : m_zdo(zdo), m_member(&member) {}

    public:
        bool valid() const {
            return m_member != nullptr;
        }

        const T& value() const {
            if (valid())
                return *this->m_member;
            else
                throw std::runtime_error("cannot retrieve T& from null");
        }

        void operator=(const T& other) {
            if (valid()) {
                // Only revise if types are unequal (if an actual noticeable change will happen)
                if ((!std::is_same_v<T, BYTES_t> && !std::is_same_v<T, std::string>) 
                    || *this->m_member != other) {
                    *this->m_member = other;
                    m_zdo.Revise();
                }
            }
            else {
                throw std::runtime_error("cannot reassign null type");
            }
        }
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
        else if constexpr (std::same_as<T, Vector3>) {
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

#ifdef RUN_TESTS // hmm
public:
#endif
    template<TrivialSyncType T>
    static constexpr SHIFTHASH_t ToShiftHash(HASH_t hash) {
        size_t key = std::hash<Ordinal>{}(GetOrdinal<T>());
                
        auto mut = static_cast<SHIFTHASH_t>(hash);

        // mutate deterministically so that this process is reversible
        mut ^= key;
        mut ^= ((key >> 0) & 0xFF) << 56;
        //mut ^= ((mut >> 7) & 0xFF) << 28;
        mut ^= ((key >> 14) & 0xFF) << 14;
        //mut ^= ((mut >> 28) & 0xFF) << 7;
        mut ^= ((key >> 56) & 0xFF) << 0;
        mut ^= key;

        return mut;
    }

    template<TrivialSyncType T>
    static constexpr HASH_t FromShiftHash(SHIFTHASH_t hash) {
        size_t key = std::hash<Ordinal>{}(GetOrdinal<T>());

        auto mut = static_cast<SHIFTHASH_t>(hash);

        mut ^= key;
        mut ^= ((key >> 56) & 0xFF) << 0;
        //mut ^= ((mut >> 28) & 0xFF) << 7;
        mut ^= ((key >> 14) & 0xFF) << 14;
        //mut ^= ((mut >> 7) & 0xFF) << 28;
        mut ^= ((key >> 0) & 0xFF) << 56;
        mut ^= key;
        
        return static_cast<HASH_t>(mut & 0xFFFFFFFF);
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

    public:
        template<TrivialSyncType T>
        Ord(const T &type) {
            this->m_contiguous = (BYTE_t*)malloc(sizeof(Ordinal) + sizeof(T));

            *this->_Ordinal() = GetOrdinal<T>();

            // https://stackoverflow.com/questions/2494471/c-is-it-possible-to-call-a-constructor-directly-without-new
            new (this->_Member<T>()) T(type);
        }

        Ord(const Ord& other) {
            const auto ord = *other._Ordinal();
            switch (ord) {
            case ORD_FLOAT:         this->m_contiguous = (BYTE_t*)malloc(sizeof(Ordinal) + sizeof(float));          *_Member<float>() = *other._Member<float>(); break;
            case ORD_VECTOR3:       this->m_contiguous = (BYTE_t*)malloc(sizeof(Ordinal) + sizeof(Vector3));        *_Member<Vector3>() = *other._Member<Vector3>(); break;
            case ORD_QUATERNION:    this->m_contiguous = (BYTE_t*)malloc(sizeof(Ordinal) + sizeof(Quaternion));     *_Member<Quaternion>() = *other._Member<Quaternion>(); break;
            case ORD_INT:           this->m_contiguous = (BYTE_t*)malloc(sizeof(Ordinal) + sizeof(int32_t));        *_Member<int32_t>() = *other._Member<int32_t>(); break;
            case ORD_LONG:          this->m_contiguous = (BYTE_t*)malloc(sizeof(Ordinal) + sizeof(int64_t));        *_Member<int64_t>() = *other._Member<int64_t>(); break;
            case ORD_STRING:        this->m_contiguous = (BYTE_t*)malloc(sizeof(Ordinal) + sizeof(std::string));    new (this->_Member<std::string>()) std::string(*other._Member<std::string>()); break;
            case ORD_ARRAY:         this->m_contiguous = (BYTE_t*)malloc(sizeof(Ordinal) + sizeof(BYTES_t));        new (this->_Member<BYTES_t>()) BYTES_t(*other._Member<BYTES_t>()); break;
            default:
                assert(false && "reached impossible case");
            }

            *this->_Ordinal() = ord;
        }

        Ord(Ord&& other) noexcept {
            this->m_contiguous = other.m_contiguous;
            other.m_contiguous = nullptr;
        }

        ~Ord() {
            if (!m_contiguous)
                return;

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



        template<TrivialSyncType T>
        bool IsType() const {
            return *_Ordinal() == GetOrdinal<T>();
        }

        // Ensure the underlying type matches
        //  Will throw on type mismatch
        template<TrivialSyncType T>
        void AssertType() const {
            //assert(IsType<T>() && "type has collision; bad algo or peer zdo is malicious");
            if (!IsType<T>())
                throw VUtils::data_error("zdo typemask mismatch");
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
            case ORD_FLOAT: return sizeof(Ordinal) + sizeof(float);
            case ORD_VECTOR3: return sizeof(Ordinal) + sizeof(Vector3);
            case ORD_QUATERNION: return sizeof(Ordinal) + sizeof(Quaternion);
            case ORD_INT: return sizeof(Ordinal) + sizeof(int32_t);
            case ORD_LONG: return sizeof(Ordinal) + sizeof(int64_t);
            case ORD_STRING: return sizeof(Ordinal) + sizeof(std::string) + _Member<std::string>()->capacity();
            case ORD_ARRAY: return sizeof(Ordinal) + sizeof(BYTES_t) + _Member<BYTES_t>()->capacity();
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
        case ORD_VECTOR3:		return _Set(key, *(Vector3*)       value);
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



public:     OWNER_t m_owner = 0;
private:    const Prefab* m_prefab = nullptr;
private:    Quaternion m_rotation = Quaternion::IDENTITY;
private:    robin_hood::unordered_map<SHIFTHASH_t, Ord> m_members;
private:    Ordinal m_ordinalMask = 0;
private:    Vector3 m_position;

public:     Rev m_rev = {};
public:     NetID m_id;


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
                        writer.WriteChar(count);
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
    void _TryReadType(DataReader& reader) {
        CountType count = sizeof(CountType) == 2 ? reader.ReadChar() : reader.Read<BYTE_t>();

        for (int i=0; i < count; i++) {
            // ...fuck
            // https://stackoverflow.com/questions/2934904/order-of-evaluation-in-c-function-parameters
            auto hash(reader.Read<HASH_t>());
            auto type(reader.Read<T>());
            _Set(hash, type);
        }
    }

public:
    ZDO() = default;

    // ZDOManager constructor
    ZDO(const NetID& id, const Vector3& pos);

    ZDO(const ZDO& other) = default;

public:
    // Save ZDO to disk
    void Save(DataWriter& writer) const;

    // Load ZDO from disk
    //  Returns whether this ZDO is modern
    bool Load(DataReader& reader, int32_t version);

    // Get a member by hash
    //  Returns a mutable proxy to the object with changes accurately reflected
    //  Throws on type mismatch
    template<TrivialSyncType T>
    ProxyMember<T> ProxyGet(HASH_t key) {
        if (m_ordinalMask & GetOrdinalMask<T>()) {
            auto mut = ToShiftHash<T>(key);
            auto&& find = m_members.find(mut);
            if (find != m_members.end()) {
                return ProxyMember<T>(*this, *find->second.Get<T>());
            }
        }
        return ProxyMember<T>(*this);
    }

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

    // Trivial hash getters
    template<TrivialSyncType T>
        //requires (!std::same_as<T, BYTES_t>)    // Bytes has no default value for missing entries
    const T& Get(HASH_t key, const T& value) const {
        auto&& get = Get<T>(key);
        if (get) return *get;
        return value;
    }

    float GetFloat(HASH_t key, float value) const;
    int32_t GetInt(HASH_t key, int32_t value) const;
    int64_t GetLong(HASH_t key, int64_t value) const;
    const Quaternion& GetQuaternion(HASH_t key, const Quaternion& value) const;
    const Vector3& GetVector3(HASH_t key, const Vector3& value) const;
    const std::string& GetString(HASH_t key, const std::string& value) const;
    const BYTES_t* GetBytes(HASH_t key /* no default */) const;

    // Special hash getters

    bool GetBool(HASH_t key, bool value) const;
    NetID GetNetID(const std::pair<HASH_t, HASH_t>& key /* no default */) const;



    //template<TrivialSyncType T>
    //    requires (!std::same_as<T, BYTES_t>)
    //const T& Get(const std::string& key, const T& value) {
    //    return Get(VUtils::String::GetStableHashCode(key), value);
    //}

    // Trivial string getters

    float GetFloat(const std::string& key, float value) const;
    int32_t GetInt(const std::string& key, int32_t value) const;
    int64_t GetLong(const std::string& key, int64_t value) const;
    const Quaternion& GetQuaternion(const std::string& key, const Quaternion& value) const;
    const Vector3& GetVector3(const std::string& key, const Vector3& value) const;
    const std::string& GetString(const std::string& key, const std::string& value) const;
    const BYTES_t* GetBytes(const std::string& key /* no default */);

    // Special string getters

    bool GetBool(const std::string& key, bool value) const;
    NetID GetNetID(const std::string& key /* no default */) const;



    // Trivial hash setters

    template<TrivialSyncType T>
    void Set(HASH_t key, const T& value) {
        if (_Set(key, value))
            Revise();
    }

    // Special hash setters

    void Set(HASH_t key, bool value);
    void Set(const std::pair<HASH_t, HASH_t>& key, const NetID& value);



    // Trivial string setters (+bool)

    template<typename T> requires TrivialSyncType<T> || std::same_as<T, bool>
    void Set(const std::string & key, const T & value) { Set(VUtils::String::GetStableHashCode(key), value); }
    void Set(const std::string& key, const std::string& value) { Set(VUtils::String::GetStableHashCode(key), value); } // String overload
    void Set(const char* key, const std::string& value) { Set(VUtils::String::GetStableHashCode(key), value); } // string constexpr overload

    // Special string setters

    void Set(const std::string& key, const NetID& value) { Set(ToHashPair(key), value); }
    void Set(const char* key, const NetID& value) { Set(ToHashPair(key), value); }







    // dumb vars
    //float m_tempSortValue = 0; // only used in sending priority
    //bool m_tempHaveRevision = 0; // appears to be unused besides assignments

    //int32_t m_tempRemovedAt = -1; // equal to frame counter at intervals
    //int32_t m_tempCreatedAt = -1; // ^

    //bool Persists() const {
    //    return m_persistent;
    //}
    //
    //bool Distant() const {
    //    return m_distant;
    //}

    //int32_t Version() const {
    //    return m_pgwVersion;
    //}

    //ObjectType Type() const {
    //    return m_type;
    //}

    const Prefab* GetPrefab() const {
        return m_prefab;
    }

    const Quaternion& Rotation() const {
        return m_rotation;
    }

    Vector2i Sector() const;

    const NetID ID() const {
        return m_id;
    }

    //const Rev GetRev() const {
    //    return m_rev;
    //}

    OWNER_t Owner() const {
        return m_owner;
    }

    const Vector3& Position() const {
        return m_position;
    }

    void SetPosition(const Vector3& pos);

    // Return whether the ZDO instance is self hosted or remotely hosted
    bool Local() const;

    // Whether an owner has been assigned to this ZDO
    bool HasOwner() const {
        return m_owner != 0;
    }

    // Claim ownership over this ZDO
    bool SetLocal();

    // set the owner of the ZDO
    bool SetOwner(OWNER_t owner) {
        // only if the owner has changed, then revise it
        if (m_owner != owner) {
            m_owner = owner;
            m_rev.m_ownerRev++;
            return true;
        }
        return false;
    }

    bool Valid() const {
        if (m_id)
            return true;
        return false;
    }

    // Should name better
    void Abandon() {
        SetOwner(0);
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
