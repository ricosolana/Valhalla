#pragma once

#include <robin_hood.h>
#include <type_traits>

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
//class Prefab;

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

    static std::pair<HASH_t, HASH_t> ToHashPair(const std::string& key);

private:
    // https://stackoverflow.com/a/1122109
    //enum class Ordinal : uint8_t {
    //    FLOAT = 1,  // 1 << (1 - 1) = 1
    //    VECTOR3,    // 1 << (2 - 1) = 2
    //    QUATERNION, // 1 << (3 - 1) = 4
    //    INT,        // 1 << (4 - 1) = 8
    //    STRING,     // 1 << (5 - 1) = 16
    //    LONG = 7,   // 1 << (7 - 1) = 64
    //    ARRAY,      // 1 << (8 - 1) = 128
    //};

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



    template<TrivialSyncType T>
    static constexpr HASH_t ToShiftHash(HASH_t hash) {
        constexpr auto shift = GetOrdinal<T>();

        return (hash
            + (shift * shift)
            ^ shift)
            ^ (shift << shift);
    }

    template<TrivialSyncType T>
    static constexpr HASH_t FromShiftHash(HASH_t hash) {
        constexpr auto shift = GetOrdinal<T>();

        return
            ((hash
                ^ (shift << shift))
                ^ shift)
            - (shift * shift);
    }



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

        Ord(const Ord& other) = delete;

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
        //  Will throw on type mismatch
        template<TrivialSyncType T>
        void Set(const T& type) {
            AssertType<T>();

            *_Member<T>() = type;
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
        bool Write(DataWriter& writer, HASH_t shiftHash) const {
            if (!IsType<T>())
                return false;

            writer.Write(FromShiftHash<T>(shiftHash));
            writer.Write(*_Member<T>());
            return true;
        }
    };



    // Get the object by hash
    // No copies are made (even for primitives)
    // Returns null if not found 
    // Throws on type mismatch
    template<TrivialSyncType T>
    const T* _Get(HASH_t key) const {
        if (m_ordinalMask & GetOrdinalMask<T>()) {
            key = ToShiftHash<T>(key);
            auto&& find = m_members.find(key);
            if (find != m_members.end()) {
                return find->second.Get<T>();
            }
        }
        return nullptr;
    }

    // Set the object by hash
    // If the hash already exists (assuming type matches), type assignment operator is used
    // Otherwise, a copy is made of the value and allocated
    // Throws on type mismatch
    template<TrivialSyncType T>
    void _Set(HASH_t key, const T& value) {
        key = ToShiftHash<T>(key);

        // Quickly check whether type is in map
        if (m_ordinalMask & GetOrdinalMask<T>()) {

            // Check whether the exact hash is in map
            //  If map contains, assumed a value reassignment (of same type)
            auto&& find = m_members.find(key);
            if (find != m_members.end()) {
                find->second.Set<T>(value);
                return;
            }
        }
        else {
            m_ordinalMask |= GetOrdinalMask<T>();
        }
        m_members.insert({ key, Ord(value) });
    }

    void _Set(HASH_t key, const void* value, Ordinal ordinal) {
        switch (ordinal) {
        case ORD_FLOAT:		    _Set(key, *(float*)         value); break;
        case ORD_VECTOR3:		_Set(key, *(Vector3*)       value); break;
        case ORD_QUATERNION:	_Set(key, *(Quaternion*)    value); break;
        case ORD_INT:			_Set(key, *(int32_t*)       value); break;
        case ORD_LONG:			_Set(key, *(int64_t*)       value); break;
        case ORD_STRING:		_Set(key, *(std::string*)   value); break;
        case ORD_ARRAY:		    _Set(key, *(BYTES_t*)       value); break;
        default:
            // good programming and proper use will prevent this case
            assert(false);
        }
    }



public:     Rev m_rev = {};
private:    robin_hood::unordered_map<HASH_t, Ord> m_members;
private:    Quaternion m_rotation = Quaternion::IDENTITY;
private:    Vector3 m_position;
private:    Ordinal m_ordinalMask = 0;
public:     OWNER_t m_owner = 0;            // local or remote OWNER_t
private:    HASH_t m_prefab = 0;
public:     NetID m_id;                    // unique identifier; immutable through 'lifetime'
public:     ObjectType m_type = ObjectType::Default; // set by ZNetView
public:     bool m_persistent = false;    // set by ZNetView
public:     bool m_distant = false;        // set by ZNetView
          
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
            //  char: null '\0' byte

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
                        writer.Write(count); // basic write in place
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

        //const auto count = reader.ReadChar();

        // The ZDO's which use many members are dungeons... (rooms index...)
        //assert(count <= 127); // TODO add try-catch (or better, handle utf8 correctly)

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

public:
    // Save ZDO to disk
    void Save(DataWriter& writer) const;

    // Load ZDO from disk
    //  Returns whether this ZDO is modern
    bool Load(DataReader& reader, int32_t version);

    // Trivial hash getters
    template<TrivialSyncType T>
        requires (!std::same_as<T, BYTES_t>)    // Bytes has no default value for missing entries
    const T& Get(HASH_t key, const T& value) const {
        auto&& get = _Get<T>(key);
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

    bool GetBool(HASH_t key, bool value = false) const;
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
        _Set(key, value);

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

    ObjectType Type() const {
        return m_type;
    }

    HASH_t PrefabHash() const {
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

    //void InvalidateSector();

    //    this->SetSector(Vector2i(-100000, -100000));
    //}

    //void SetSector(const Vector2i& sector);



    void SetPosition(const Vector3& pos);

    //bool Outdated(const Rev& min) const {
    //    return min.m_dataRev > m_rev.m_dataRev
    //        || min.m_ownerRev > m_rev.m_ownerRev;
    //}



    //friend void NetSyncManager::RPC_NetSyncData(NetRpc* rpc, NetPackage pkg);



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
        //m_owner = 0;
        SetOwner(0);
    }

    // Save ZDO to network packet
    void Serialize(DataWriter& pkg) const;

    // Load ZDO from network packet
    void Deserialize(DataReader& pkg);
};
