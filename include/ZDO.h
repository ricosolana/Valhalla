#pragma once

#include <robin_hood.h>
#include <type_traits>
#include <any>

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

// NetSync is 500+ bytes (with all 7 maps)
// NetSync is 168 bytes (with 1 map only)
// NetSync is 144 bytes (combined member map, reduced members)
// Currently 160 bytes
class ZDO {
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
    enum class Ordinal : uint8_t {
        FLOAT = 1,  // 1 << (1 - 1) = 1
        VECTOR3,    // 1 << (2 - 1) = 2
        QUATERNION, // 1 << (3 - 1) = 4
        INT,        // 1 << (4 - 1) = 8
        STRING,     // 1 << (5 - 1) = 16
        LONG = 7,   // 1 << (7 - 1) = 64
        ARRAY,      // 1 << (8 - 1) = 128
    };

    template<TrivialSyncType T>
    constexpr Ordinal GetOrdinal() {
        if constexpr (std::same_as<T, float>) {
            return Ordinal::FLOAT;
        }
        else if constexpr (std::same_as<T, Vector3>) {
            return Ordinal::VECTOR3;
        }
        else if constexpr (std::same_as<T, Quaternion>) {
            return Ordinal::QUATERNION;
        }
        else if constexpr (std::same_as<T, int32_t>) {
            return Ordinal::INT;
        }
        else if constexpr (std::same_as<T, int64_t>) {
            return Ordinal::LONG;
        }
        else if constexpr (std::same_as<T, std::string>) {
            return Ordinal::STRING;
        }
        else { //if constexpr (std::same_as<T, BYTES_t>) {
            return Ordinal::ARRAY;
        }
    }

    template<TrivialSyncType T>
    constexpr Ordinal GetOrdinalShift() {
        return static_cast<std::underlying_type_t>(GetOrdinal<T>()) - 1;
    }

    template<TrivialSyncType T>
    constexpr HASH_t ToShiftHash(HASH_t hash) {
        auto shift = GetOrdinalShift<T>();

        return (hash
            + (shift * shift)
            ^ shift)
            ^ (shift << shift);
    }

    template<TrivialSyncType T>
    constexpr HASH_t FromShiftHash(HASH_t hash) {
        auto shift = GetOrdinalShift<T>();

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
    const T* _Get(HASH_t key) {
        auto ordinal = GetOrdinal<T>();
        if (m_ordinalMask(ordinal)) {
            key = ToShiftHash<T>(key);
            auto&& find = m_members.find(key);
            if (find != m_members.end()) {
                // good programming and proper use will prevent this bad case
                assert(find->second.first == ordinal);

                return (T*)find->second.second;
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
        //auto prefix = GetShift<T>();
        auto ordinal = GetOrdinal<T>();
        key = ToShiftHash<T>(key);
        if (m_ordinalMask(ordinal)) {
            auto&& find = m_members.find(key);
            if (find != m_members.end()) {
                // good programming and proper use will prevent this bad case
                assert(find->second.first == ordinal);

                // reassign if changed
                auto&& v = (T*)find->second.second;
                if (*v == value)
                    return;
                *v = value;
            }
            else {
                // TODO restructure this ugly double code part
                // could use goto, but ehh..
                m_members.insert({ key, std::make_pair(ordinal, new T(value)) });
            }
        }
        else {
            m_members.insert({ key, std::make_pair(ordinal, new T(value)) });
        }

        m_dataMask |= (0b1 << GetOrdinalShift<T>());
    }

    void _Set(HASH_t key, const void* value, Ordinal ordinal) {
        switch (ordinal) {
        case Ordinal::FLOAT:		_Set(key, *(float*)         value); break;
        case Ordinal::VECTOR3:		_Set(key, *(Vector3*)       value); break;
        case Ordinal::QUATERNION:	_Set(key, *(Quaternion*)    value); break;
        case Ordinal::INT:			_Set(key, *(int32_t*)       value); break;
        case Ordinal::LONG:			_Set(key, *(int64_t*)       value); break;
        case Ordinal::STRING:		_Set(key, *(std::string*)   value); break;
        case Ordinal::ARRAY:		_Set(key, *(BYTES_t*)       value); break;
        default:
            // good programming and proper use will prevent this case
            assert(false);
        }
    }

private:    
    robin_hood::unordered_map<HASH_t, std::pair<Ordinal, void*>> m_members;

    bool m_persistent = false;    // set by ZNetView
    bool m_distant = false;        // set by ZNetView
    //int64_t m_timeCreated = 0;    // TerrainModifier (10000 ticks / ms) (union)?
    
    ObjectType m_type = ObjectType::Default; // set by ZNetView
    HASH_t m_prefab = 0;
    Quaternion m_rotation = Quaternion::IDENTITY;
    BitMask<Ordinal> m_ordinalMask;

    Vector2i m_sector;
    Vector3 m_position;            // position of the 
    NetID m_id;                    // unique identifier; immutable through 'lifetime'
    OWNER_t m_owner = 0;            // local or remote OWNER_t

    //uint32_t m_ownerRev = 0;    // could be rev structure
    //uint32_t m_dataRev = 0;    // could be rev structure

    void Revise() {
        m_rev.m_dataRev++;
    }

    void FreeMembers();

    template<typename T>
    bool _TryWriteType(NetPackage& pkg) {
        //if ((m_dataMask >> static_cast<uint8_t>(GetShift<T>())) & 0b1) {
        if (m_ordinalMask(GetOrdinal<T>())) {
            // Save structure per each type:
            //  char: count
            //      string: key
            //      F V Q I L S A: value
            //  char: null '\0' byte

            auto size_mark = pkg.m_stream.Position();
            BYTE_t count = 0;
            pkg.Write(count); // seek forward 1 dummy byte (faster than iterating entire map beforehand)
            for (auto&& pair : m_members) {
                // find the matching types
                if (pair.second.first != GetOrdinal<T>())
                    continue;
                pkg.Write(FromShiftHash<T>(pair.first));
                pkg.Write(*(T*) pair.second.second);
                count++;
            }
            auto end_mark = pkg.m_stream.Position();
            pkg.m_stream.SetPos(size_mark);
            pkg.Write(count);
            pkg.m_stream.SetPos(end_mark);

            return true;
        }
        return false;
    }

    template<typename T>
    bool _TryReadType(NetPackage& pkg) {
        auto count = pkg.Read<BYTE_t>();
        if (count) {
            while (count--) {
                _Set(pkg.Read<HASH_t>(), pkg.Read<T>());
            }
            // TODO return condition might be redundant
            return true;
        }
        return false;
    }

public:
    Rev m_rev;
    int32_t m_pgwVersion = 0;    // 53 is the latest

public:
    // Create ZDO with me (im owner)
    ZDO();

    // Loading ZDO from disk package
    //ZDO(NetPackage reader, int version);

    // Save ZDO to the disk package
    void Save(NetPackage& writer);

    void Load(NetPackage& reader, int32_t version);


    ZDO(const ZDO& other); // copy constructor

    ~ZDO();

    // Trivial hash getters

    template<TrivialSyncType T>
        requires (!std::same_as<T, BYTES_t>)    // Bytes has no default value for missing entries
    const T& Get(HASH_t key, const T& value) {
        auto&& get = _Get<T>(key);
        if (get) return *get;
        return value;
    }

    float GetFloat(HASH_t key, float value = 0);
    int32_t GetInt(HASH_t key, int32_t value = 0);
    int64_t GetLong(HASH_t key, int64_t value = 0);
    const Quaternion& GetQuaternion(HASH_t key, const Quaternion& value = Quaternion::IDENTITY);
    const Vector3& GetVector3(HASH_t key, const Vector3& value);
    const std::string& GetString(HASH_t key, const std::string& value = "");
    const BYTES_t* GetBytes(HASH_t key /* no default */);

    // Special hash getters

    bool GetBool(HASH_t key, bool value = false);
    NetID GetNetID(const std::pair<HASH_t, HASH_t>& key /* no default */);



    //template<TrivialSyncType T>
    //    requires (!std::same_as<T, BYTES_t>)
    //const T& Get(const std::string& key, const T& value) {
    //    return Get(VUtils::String::GetStableHashCode(key), value);
    //}

    // Trivial string getters

    float GetFloat(const std::string& key, float value = 0);
    int32_t GetInt(const std::string& key, int32_t value = 0);
    int64_t GetLong(const std::string& key, int64_t value = 0);
    const Quaternion& GetQuaternion(const std::string& key, const Quaternion& value = Quaternion::IDENTITY);
    const Vector3& GetVector3(const std::string& key, const Vector3& value);
    const std::string& GetString(const std::string& key, const std::string& value = "");
    const BYTES_t* GetBytes(const std::string& key /* no default */);

    // Special string getters

    bool GetBool(const std::string& key, bool value = false);
    NetID GetNetID(const std::string& key /* no default */);



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
    bool Local();

    // Whether an owner has been assigned to this ZDO
    bool HasOwner() const {
        return m_owner != 0;
    }

    // Claim ownership over this ZDO
    void SetLocal();

    // set the owner of the ZDO
    void SetOwner(OWNER_t owner) {
        if (m_owner != owner) {
            m_owner = owner;
            m_rev.m_ownerRev++;
        }
    }

    bool Valid() const {
        if (m_id)
            return true;
        return false;
    }

    void Invalidate();

    void Abandon() {
        m_owner = 0;
    }

    // Write ZDO to the network packet
    void Serialize(NetPackage& pkg);

    // Load ZDO from the network packet
    void Deserialize(NetPackage& pkg);
};
