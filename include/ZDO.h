#pragma once

#include <robin_hood.h>
#include <type_traits>

#include "HashUtils.h"
#include "Quaternion.h"
#include "Vector.h"
#include "VUtils.h"
#include "VUtilsString.h"
#include "NetPackage.h"

template<typename T>
concept TrivialSyncType = 
       std::same_as<T, float>
    || std::same_as<T, Vector3>
    || std::same_as<T, Quaternion>
    || std::same_as<T, int32_t>
    || std::same_as<T, int64_t>
    || std::same_as<T, std::string>
    || std::same_as<T, BYTES_t>;

class ZDOPeer;
class IZDOManager;

// 500+ bytes (7 maps)
// 168 bytes (1 map)
// 112 bytes (1 map, majorly reduced members; affecting functionality)
// Currently 144 bytes
class ZDO {
    friend class ZDOPeer;
    friend class IZDOManager;

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
        int64_t m_time = 0;
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
                // good programming and proper use will prevent this bad case
                assert(find->second.first == GetOrdinal<T>());

                return (T*) find->second.second;
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
        if (m_ordinalMask & GetOrdinalMask<T>()) {
            auto&& find = m_members.find(key);
            if (find != m_members.end()) {
                // good programming and proper use will prevent this bad case
                assert(find->second.first == GetOrdinal<T>());

                // reassign if changed
                auto&& v = (T*) find->second.second;
                if (*v == value)
                    return;
                *v = value;
            }
            else {
                // TODO restructure this ugly double code part
                // could use goto, but ehh..
                m_members.insert({ key, std::make_pair(GetOrdinal<T>(), new T(value)) });
            }
        }
        else {
            m_ordinalMask |= GetOrdinalMask<T>();
            m_members.insert({ key, std::make_pair(GetOrdinal<T>(), new T(value)) });
        }
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

public:
    Rev m_rev;
    //int32_t m_pgwVersion = 0;    // 53 is the latest      // Never used beyond world startup

private:
    robin_hood::unordered_map<HASH_t, std::pair<Ordinal, void*>> m_members;

    Quaternion m_rotation = Quaternion::IDENTITY;
    Vector3 m_position;            // position of the 
    
    //int e; // INSERT HERE
    //uint8_t m_flags;                      // flags: persistent, distant, type (2 bits)
    Ordinal m_ordinalMask = 0;
    OWNER_t m_owner = 0;            // local or remote OWNER_t
    //OWNER_t m_zdoid_userid = 0;
    HASH_t m_prefab = 0;
    
    
    //uint32_t m_zdoid_id = 0;



    NetID m_id;                    // unique identifier; immutable through 'lifetime'
    Vector2i m_sector;        // Redundant; is based directly off position
    
    ObjectType m_type = ObjectType::Default; // set by ZNetView

    //Ordinal m_ordinalMask = 0; // bitfield denoting which map member types this ZDO contains



    bool m_persistent = false;    // set by ZNetView
    bool m_distant = false;        // set by ZNetView
    





    //int64_t m_timeCreated = 0;    // TerrainModifier (10000 ticks / ms) (union)?

    //uint32_t m_ownerRev = 0;    // could be rev structure
    //uint32_t m_dataRev = 0;    // could be rev structure

    void Revise() {
        m_rev.m_dataRev++;
    }

    void FreeMembers();

    template<typename T, typename CountType>
    void _TryWriteType(NetPackage& pkg) const {
        // Load/Save use count char for every member (including 0 counts)
        //assert(false && "fix me please; c# BinaryWriter::Write(char) writes a dynamic amount of bytes");

        if constexpr (sizeof(CountType) == 2)
            pkg.Write((BYTE_t)0); // placeholder byte; also serves as 0 count when type T is absent        

        if (m_ordinalMask & GetOrdinalMask<T>()) {
            // Save structure per each type:
            //  char: count
            //      string: key
            //      F V Q I L S A: value
            //  char: null '\0' byte

            if constexpr (sizeof(CountType) == 1)
                pkg.Write((BYTE_t)0); // placeholder byte; also serves as 0 count when type T is absent
                        
            const auto size_mark = pkg.m_stream.Position() - sizeof(BYTE_t);
            BYTE_t count = 0;
            for (auto&& pair : m_members) {
                // skip any types not matching
                if (pair.second.first != GetOrdinal<T>())
                    continue;

                pkg.Write(FromShiftHash<T>(pair.first));
                pkg.Write(*(T*)pair.second.second);
                count++;

                assert(count <= 127 && "shit");
            }

            const auto end_mark = pkg.m_stream.Position();
            pkg.m_stream.SetPos(size_mark);

            pkg.Write(count);

            pkg.m_stream.SetPos(end_mark);
        }
    }

    template<typename T, typename CountType>
    void _TryReadType(NetPackage& pkg) {
        auto count = pkg.Read<BYTE_t>();

        assert(count <= 127);        

        while (count--) {
            // ...fuck
            // https://stackoverflow.com/questions/2934904/order-of-evaluation-in-c-function-parameters
            auto hash(pkg.Read<HASH_t>());
            auto type(pkg.Read<T>());
            _Set(hash, type);
        }
    }



public:
    // Create ZDO with me (im owner)
    ZDO();

    // Loading ZDO from disk package
    //ZDO(NetPackage reader, int version);

    // Save ZDO to the disk package
    void Save(NetPackage& writer) const;

    void Load(NetPackage& reader, int32_t version);


    ZDO(const ZDO& other); // copy constructor

    ~ZDO();

    // Trivial hash getters

    template<TrivialSyncType T>
        requires (!std::same_as<T, BYTES_t>)    // Bytes has no default value for missing entries
    const T& Get(HASH_t key, const T& value) const {
        auto&& get = _Get<T>(key);
        if (get) return *get;
        return value;
    }

    float GetFloat(HASH_t key, float value = 0) const;
    int32_t GetInt(HASH_t key, int32_t value = 0) const;
    int64_t GetLong(HASH_t key, int64_t value = 0) const;
    const Quaternion& GetQuaternion(HASH_t key, const Quaternion& value = Quaternion::IDENTITY) const;
    const Vector3& GetVector3(HASH_t key, const Vector3& value) const;
    const std::string& GetString(HASH_t key, const std::string& value = "") const;
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

    float GetFloat(const std::string& key, float value = 0) const;
    int32_t GetInt(const std::string& key, int32_t value = 0) const;
    int64_t GetLong(const std::string& key, int64_t value = 0) const;
    const Quaternion& GetQuaternion(const std::string& key, const Quaternion& value = Quaternion::IDENTITY) const;
    const Vector3& GetVector3(const std::string& key, const Vector3& value) const;
    const std::string& GetString(const std::string& key, const std::string& value = "") const;
    const BYTES_t* GetBytes(const std::string& key /* no default */);

    // Special string getters

    bool GetBool(const std::string& key, bool value = false) const;
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

    bool Persists() const {
        return m_persistent;
    }

    bool Distant() const {
        return m_distant;
    }

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

     const Vector2i& Sector() const {
        //return IZoneManager::WorldToZonePos(m_position);
         return m_sector;
    }

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

    void InvalidateSector() {
        this->SetSector(Vector2i(-100000, -10000));
    }

    void SetSector(const Vector2i& sector);



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

    void Invalidate();

    // Should name better
    void Abandon() {
        //m_owner = 0;
        SetOwner(0);
    }

    // Save ZDO to network packet
    void Serialize(NetPackage& pkg) const;

    // Load ZDO from network packet
    void Deserialize(NetPackage& pkg);
};

using NetSync = ZDO;
