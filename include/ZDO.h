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
#include "Types.h"

// The 'butter' of Valheim
// This class has been refactored numerous times 
//  Performance is important but memory usage has been highly prioritized here
// This class used to be 500+ bytes
//  It is now 120 bytes 
// This class is finally the smallest it could possibly be (I hope so).
//  I Lied this class is now 36 bytes 5/24/2023
//      On embedded systems with max 8MB of RAM (ESP32), ZDOID can be smaller
//          
class ZDO {
    friend class IZDOManager;
    friend class IPrefabManager;
    friend class VHTest;
    friend class IValhalla;



    using member_hash = uint64_t;

    using member_tuple = std::tuple<float, Vector3f, Quaternion, int32_t, int64_t, std::string, BYTES_t>;
    using member_variant = VUtils::Traits::tuple_to_variant<member_tuple>::type;

    using member_map = UNORDERED_MAP_t<member_hash, member_variant>;



    template<typename T>
    using is_member = VUtils::Traits::tuple_has_type<T, member_tuple>;

    template<typename T> 
    static constexpr bool is_member_v = is_member<T>::value;

    template<typename T>
        requires is_member<T>::value
    using member_denotion = std::integral_constant<size_t, VUtils::Traits::tuple_index<T, member_tuple>::value>;

    template<typename T>
    static constexpr size_t member_denotion_v = member_denotion<T>::value;

    template<typename T>
        requires is_member<T>::value
    using member_flag = std::integral_constant<size_t, 1 << member_denotion_v<T>>;

    template<typename T>
    static constexpr size_t member_flag_v = member_flag<T>::value;

public:
    using reference = std::reference_wrapper<ZDO>;

    class Rev {
    private:
        // DataRevision: 0, OwnerRevision: 1
        BitPack<uint32_t, 21, 32 - 21> m_pack;

        static constexpr auto DATA_REVISION_PACK_INDEX = 0;
        static constexpr auto OWNER_REVISION_PACK_INDEX = 1;

    public:
        Rev() {}

        Rev(uint32_t dataRev, uint16_t ownerRev) {
            SetDataRevision(dataRev);
            SetOwnerRevision(ownerRev);
        }

        [[nodiscard]] uint32_t GetDataRevision() const {
            return m_pack.Get<DATA_REVISION_PACK_INDEX>();
        }

        [[nodiscard]] uint16_t GetOwnerRevision() const {
            return m_pack.Get<OWNER_REVISION_PACK_INDEX>();
        }



        void SetDataRevision(uint32_t dataRev) {
            m_pack.Set<DATA_REVISION_PACK_INDEX>(dataRev);
        }

        void SetOwnerRevision(uint16_t ownerRev) {
            m_pack.Set<OWNER_REVISION_PACK_INDEX>(ownerRev);
        }



        void ReviseData() {
            this->SetDataRevision(GetDataRevision() + 1);
        }

        void ReviseOwner() {
            this->SetOwnerRevision(GetOwnerRevision() + 1);
        }
    };

private:
    static constexpr uint32_t MACHINE_Persistent = 0;
    static constexpr uint32_t MACHINE_Distant = 1;
    static constexpr uint32_t MACHINE_Type1 = 2;
    static constexpr uint32_t MACHINE_Type2 = 3;

    static constexpr uint32_t NETWORK_Connection = 0;
    static constexpr uint32_t NETWORK_Float = 1;
    static constexpr uint32_t NETWORK_Vec3 = 2;
    static constexpr uint32_t NETWORK_Quat = 3;
    static constexpr uint32_t NETWORK_Int = 4;
    static constexpr uint32_t NETWORK_Long = 5;
    static constexpr uint32_t NETWORK_String = 6;
    static constexpr uint32_t NETWORK_ByteArray = 7;
    static constexpr uint32_t NETWORK_Persistent = 8;
    static constexpr uint32_t NETWORK_Distant = 9;
    static constexpr uint32_t NETWORK_Type1 = 10;
    static constexpr uint32_t NETWORK_Type2 = 11;
    static constexpr uint32_t NETWORK_Rotation = 12;

private:

    // Set the object by hash (Internal use only; does not revise ZDO on changes)
    //  Returns whether the previous value was modified
    //  Throws on type mismatch
    template<typename T>
        requires is_member<T>::value
    [[maybe_unused]] bool _Set(HASH_t key, T value, member_map& members) {
        auto mut = hash_to_xhash<T>(key);

        auto&& insert = members.insert({ mut, 0.f });
        if (insert.second) {
            // Then officially assign
            insert.first->second = std::move(value);

            //m_pack.Merge<2>(1 << member_denotion<T>::value);
            //m_pack.Merge<FLAGS_PACK_INDEX>(member_flag_v<T>);
            return true;
        }
        else {
            //assert(m_pack.Get<2>() & (1 << member_denotion<T>::value));
            //assert(m_pack.Get<FLAGS_PACK_INDEX>() & member_flag_v<T>);

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
        requires is_member_v<T>
    [[maybe_unused]] bool _Set(HASH_t key, T value) {
        return _Set(key, std::move(value), ZDO_MEMBERS[ID()]);
    }

    void Revise() {
        m_rev.SetDataRevision(m_rev.GetDataRevision() + 1);
    }

    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] member_hash hash_to_xhash(HASH_t in) const {
        return static_cast<member_hash>(in) 
            ^ static_cast<member_hash>(ankerl::unordered_dense::hash<size_t>{}(VUtils::Traits::tuple_index_v<T, member_tuple>));
    }

    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] HASH_t xhash_to_hash(member_hash in) const {
        return static_cast<HASH_t>(in) 
            ^ static_cast<HASH_t>(ankerl::unordered_dense::hash<size_t>{}(VUtils::Traits::tuple_index_v<T, member_tuple>));
    }

    template<typename T>
        requires is_member_v<T>
    decltype(auto) _TryWriteType(DataWriter& writer, member_map& members) const {
        const auto begin_mark = writer.Position();
        uint8_t count = 0;
        //writer.Write(count); // placeholder 0 byte

        for (auto&& pair : members) {
            auto&& data = std::get_if<T>(&pair.second);
            if (data) {
                // Skip 1 byte for count only if member present
                if (!count) {
                    writer.Write(count);
                }

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

        return count;
    }

    // Read a zdo_type from the DataStream
    template<typename T, typename CountType>
        requires is_member_v<T> && (std::same_as<CountType, char16_t> || std::same_as<CountType, uint8_t>)
    void _TryReadType(DataReader& reader, member_map& members) {
        decltype(auto) count = reader.Read<CountType>();

        for (int i=0; i < count; i++) {
            // ...fuck
            // https://stackoverflow.com/questions/2934904/order-of-evaluation-in-c-function-parameters
            auto hash(reader.Read<HASH_t>());
            auto type(reader.Read<T>());
            _Set(hash, type, members);
        }
    }

    void _SetPrefabHash(HASH_t hash) {
        m_prefabHash = hash;
    }

private:
    static inline ankerl::unordered_dense::segmented_map<ZDOID, member_map> ZDO_MEMBERS;
    static inline ankerl::unordered_dense::segmented_map<ZDOID, ZDOConnectorData> ZDO_CONNECTORS; // Saved typed-connectors
    static inline ankerl::unordered_dense::segmented_map<ZDOID, ZDOConnectorTargeted> ZDO_TARGETED_CONNECTORS; // Current linked connectors

    static constexpr auto OWNER_PACK_INDEX = 0;
    static constexpr auto FLAGS_PACK_INDEX = 1;

    /*
    * 36 bytes total:
    */

    Vector3f m_pos;                                 // 12 bytes
    ZDOID m_id;                                     // 4 bytes (PADDING)
    Vector3f m_rotation;                            // 12 bytes
    Rev m_rev;                                      // 4 bytes (PADDING)
    // Owner: 0, Flags: 1
    //  last 6 bits are unused/reserved for future uses
    HASH_t m_prefabHash{};
    BitPack<uint16_t, VH_USER_BITS_I_, 4,
        sizeof(uint16_t)*8 - VH_USER_BITS_I_ - 4> m_pack;

public:
    ZDO();

    // ZDOManager constructor
    ZDO(ZDOID id, Vector3f pos);

    ZDO(const ZDO& other) = default;
    ZDO(ZDO&& other) = default;
        


#if VH_IS_ON(VH_LEGACY_WORLD_COMPATABILITY)
    // Load ZDO from disk
    //  Returns whether this ZDO is modern
    void Load31Pre(DataReader& reader, int32_t version);
#endif //VH_LEGACY_WORLD_COMPATABILITY

    // Reads from a buffer using the new efficient format (version >= 31)
    //  version=0: Read according to the network deserialize format
    //  version>0: Read according to the file load format
    void Unpack(DataReader& reader, int32_t version);

    // Writes to a buffer using the new efficient format (version >= 31)
    //  If 'network' is true, write according to the network serialize format
    //  Otherwise write according to the file save format
    void Pack(DataWriter& writer, bool network) const;

    // Erases and returns the value 
    template<typename T>
        requires is_member_v<T>
    [[maybe_unused]] bool Extract(HASH_t key, T& out) {
        //if (m_pack.Get<FLAGS_PACK_INDEX>() & member_flag_v<T>) {
            auto&& members_find = ZDO_MEMBERS.find(ID());
            if (members_find != ZDO_MEMBERS.end()) {
                auto&& members = members_find->second;

                auto mut = hash_to_xhash<T>(key);

                auto&& find = members.find(mut);
                if (find != members.end()) {
                    auto&& get = std::get_if<T>(&find->second);
                    if (get) {
                        out = std::move(*get);
                        members.erase(find);
                        return true;
                    }
                }
            }
            else {
                //assert(false);
            }
        //}

        return false;
    }

    // Get a member by hash
    //  Returns null if absent 
    //  Throws on type mismatch
    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] const T* Get(HASH_t key) const {
        //static_assert(member_denotion_v<float> == std::to_underlying(LocalDenotion::Member_Float));

        //if (m_encoded.HasMember<T>()) {
        //if (m_pack.Get<2>() & (1 << GetMemberDenotion<T>())) {
        //if (m_pack.Get<FLAGS_PACK_INDEX>() & member_flag_v<T>) {
            auto&& members_find = ZDO_MEMBERS.find(ID());
            if (members_find != ZDO_MEMBERS.end()) {
                auto&& members = members_find->second;

                auto mut = hash_to_xhash<T>(key);

                auto&& find = members.find(mut);
                if (find != members.end()) {
                    return std::get_if<T>(&find->second);
                }
            }
            else {
                //assert(false);
            }
        //}

        return nullptr;
    }

    template<typename T>
        requires is_member_v<T>
    [[maybe_unused]] bool Extract(std::string_view key, T& out) {
        return Extract(VUtils::String::GetStableHashCode(key), out);
    }

    // Get a member by string
    //  Returns null if absent 
    //  Throws on type mismatch
    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] const T* Get(std::string_view key) const {
        return Get<T>(VUtils::String::GetStableHashCode(key));
    }

    // Trivial hash getters
    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] const T& Get(HASH_t key, const T& value) const {
        auto&& get = Get<T>(key);
        return get ? *get : value;
    }

    // Hash-key getters
    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] const T& Get(std::string_view key, const T &value) const { return Get<T>(VUtils::String::GetStableHashCode(key), value); }
        
    [[nodiscard]] float               GetFloat(       HASH_t key, float value) const {                            return Get<float>(key, value); }
    [[nodiscard]] int32_t             GetInt(         HASH_t key, int32_t value) const {                          return Get<int32_t>(key, value); }
    [[nodiscard]] int64_t             GetLong(        HASH_t key, int64_t value) const {                          return Get<int64_t>(key, value); }
    [[nodiscard]] Int64Wrapper        GetLongWrapper( HASH_t key, Int64Wrapper value) const {                     return Get<int64_t>(key, value); }
    [[nodiscard]] Quaternion          GetQuaternion(  HASH_t key, Quaternion value) const {                       return Get<Quaternion>(key, value); }
    [[nodiscard]] Vector3f            GetVector3(     HASH_t key, Vector3f value) const {                         return Get<Vector3f>(key, value); }
    [[nodiscard]] std::string_view    GetString(      HASH_t key, std::string_view value) const {                 auto&& val = Get<std::string>(key); return val ? std::string_view(*val) : value; }
    [[nodiscard]] const BYTES_t*      GetBytes(       HASH_t key) const {                                         return Get<BYTES_t>(key); }
    [[nodiscard]] bool                GetBool(        HASH_t key, bool value) const {                             return GetInt(key, value ? 1 : 0); }
    [[nodiscard]] ZDOID               GetZDOID(       std::pair<HASH_t, HASH_t> key, ZDOID value) const {         return ZDOID(GetLong(key.first, value.GetOwner()), GetLong(key.second, value.GetUID())); }

    // Hash-key default getters
    [[nodiscard]] float               GetFloat(       HASH_t key) const {                                         return Get<float>(key, {}); }
    [[nodiscard]] int32_t             GetInt(         HASH_t key) const {                                         return Get<int32_t>(key, {}); }
    [[nodiscard]] int64_t             GetLong(        HASH_t key) const {                                         return Get<int64_t>(key, {}); }
    [[nodiscard]] Int64Wrapper        GetLongWrapper( HASH_t key) const {                                         return Get<int64_t>(key, {}); }
    [[nodiscard]] Quaternion          GetQuaternion(  HASH_t key) const {                                         return Get<Quaternion>(key, {}); }
    [[nodiscard]] Vector3f            GetVector3(     HASH_t key) const {                                         return Get<Vector3f>(key, {}); }
    [[nodiscard]] std::string_view    GetString(      HASH_t key) const {                                         return Get<std::string>(key, {}); }
    [[nodiscard]] bool                GetBool(        HASH_t key) const {                                         return Get<int32_t>(key); }
    [[nodiscard]] ZDOID               GetZDOID(       std::pair<HASH_t, HASH_t> key) const {                      return GetZDOID(key, {}); }

    // String-key getters
    [[nodiscard]] float               GetFloat(       std::string_view key, float value) const {                  return Get<float>(key, value); }
    [[nodiscard]] int32_t             GetInt(         std::string_view key, int32_t value) const {                return Get<int32_t>(key, value); }
    [[nodiscard]] int64_t             GetLong(        std::string_view key, int64_t value) const {                return Get<int64_t>(key, value); }
    [[nodiscard]] Int64Wrapper        GetLongWrapper( std::string_view key, Int64Wrapper value) const {           return Get<int64_t>(key, value); }
    [[nodiscard]] Quaternion          GetQuaternion(  std::string_view key, Quaternion value) const {             return Get<Quaternion>(key, value); }
    [[nodiscard]] Vector3f            GetVector3(     std::string_view key, Vector3f value) const {               return Get<Vector3f>(key, value); }
    [[nodiscard]] std::string_view    GetString(      std::string_view key, std::string_view value) const {       auto&& val = Get<std::string>(key); return val ? std::string_view(*val) : value; }
    [[nodiscard]] const BYTES_t*      GetBytes(       std::string_view key) const {                               return Get<BYTES_t>(key); }
    [[nodiscard]] bool                GetBool(        std::string_view key, bool value) const {                   return Get<int32_t>(key, value); }
    [[nodiscard]] ZDOID               GetZDOID(       std::string_view key, ZDOID value) const {                  return GetZDOID(VUtils::String::ToHashPair(key), value); }

    // String-key default getters
    [[nodiscard]] float               GetFloat(       std::string_view key) const {                               return Get<float>(key, {}); }
    [[nodiscard]] int32_t             GetInt(         std::string_view key) const {                               return Get<int32_t>(key, {}); }
    [[nodiscard]] int64_t             GetLong(        std::string_view key) const {                               return Get<int64_t>(key, {}); }
    [[nodiscard]] Int64Wrapper        GetLongWrapper( std::string_view key) const {                               return Get<int64_t>(key, {}); }
    [[nodiscard]] Quaternion          GetQuaternion(  std::string_view key) const {                               return Get<Quaternion>(key, {}); }
    [[nodiscard]] Vector3f            GetVector3(     std::string_view key) const {                               return Get<Vector3f>(key, {}); }
    [[nodiscard]] std::string_view    GetString(      std::string_view key) const {                               return Get<std::string>(key, {}); }
    [[nodiscard]] bool                GetBool(        std::string_view key) const {                               return Get<int32_t>(key, {}); }
    [[nodiscard]] ZDOID               GetZDOID(       std::string_view key) const {                               return GetZDOID(key, {}); }

    // Trivial hash setters
    template<typename T>
        requires is_member_v<T>
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



    template<typename T>
        requires is_member_v<T>
    void Set(std::string_view key, T value) { Set(VUtils::String::GetStableHashCode(key), std::move(value)); }

    void Set(std::string_view key, bool value) { Set(VUtils::String::GetStableHashCode(key), value ? (int32_t)1 : 0); }

    void Set(std::string_view key, ZDOID value) { Set(VUtils::String::ToHashPair(key), value); }



    [[maybe_unused]] bool Extract(std::pair<HASH_t, HASH_t> key, ZDOID& out) {
        int64_t userID{};
        if (Extract(key.first, userID)) {
            int64_t id{};
            if (Extract(key.second, id)) {
                out = ZDOID(userID, id);
                return true;
            }
        }
        return false;
    }

    [[maybe_unused]] bool Extract(std::string_view key, ZDOID& out) {
        return Extract(VUtils::String::ToHashPair(key), out);
    }

    // Internal use
    //  Raw sets the connector with no revision
    [[maybe_unused]] bool _SetConnection(ZDOConnector::Type type, ZDOID zdoid) {
        auto&& insert = ZDO_TARGETED_CONNECTORS.insert({ ID(),
            ZDOConnectorTargeted(type, zdoid) });

        auto&& connector = insert.first->second;

        // if it was not newly inserted
        //  check the old values
        if (!insert.second) {
            auto&& type2 =  connector.m_type;
            auto&& zdoid2 = connector.m_target;

            if (type == type2 && zdoid2 == zdoid) {
                return false;
            }
        }

        connector.m_type = type;
        connector.m_target = zdoid;

        //m_pack.Merge<FLAGS_PACK_INDEX>(std::to_underlying(LocalFlag::Member_Connection));

        return true;
    }

    void SetConnection(ZDOConnector::Type type, ZDOID zdoid) {
        if (_SetConnection(type, zdoid)) {
            Revise();
        }
    }



    ZDOID GetConnectionZDOID(ZDOConnector::Type type) const {
        auto&& find = ZDO_TARGETED_CONNECTORS.find(ID());
        if (find != ZDO_TARGETED_CONNECTORS.end()) {
            if (find->second.m_type == type)
                return find->second.m_target;
        }
        return ZDOID::NONE;
    }


    [[nodiscard]] ZDOID ID() const {
        return this->m_id;
    }

    [[nodiscard]] Vector3f Position() const {
        return m_pos;
    }

    // Set the position of the ZDO
    //  - Use this method 99.9% of the time when updating the ZDO's position
    //  - This will change and invalidate sectors if the new position is in a different zone than this ZDOs position
    void SetPosition(Vector3f pos);

    [[nodiscard]] ZoneID GetZone() const;

    [[nodiscard]] Quaternion Rotation() const {
        return Quaternion::Euler(m_rotation.x, m_rotation.y, m_rotation.z);
    }

    void SetRotation(Quaternion rot) {
        auto&& euler = rot.EulerAngles();
        if (euler != m_rotation) {
            m_rotation = euler;
            Revise();
        }
    }
        
#if VH_IS_ON(VH_STANDARD_PREFABS)
    [[nodiscard]] const Prefab& GetPrefab() const {
        return PrefabManager()->RequirePrefabByHash(m_prefabHash);
    }
#endif
    
    [[nodiscard]] HASH_t GetPrefabHash() const {
        return m_prefabHash;
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

    // The owner of the ZDO
    [[nodiscard]] OWNER_t Owner() const {
        return ZDOID::GetUserIDByIndex(m_pack.Get<OWNER_PACK_INDEX>());
    }


    // Whether the ZDO is owned by a specific owner
    [[nodiscard]] bool IsOwner(OWNER_t owner) const {
        return owner == this->Owner();
    }

    // Returns whether this server is the owner of the ZDO
    [[nodiscard]] bool IsLocal() const {
        return IsOwner(VH_ID);
    }

    // Whether the ZDO has an owner
    [[nodiscard]] bool HasOwner() const {
        return m_pack.Get<OWNER_PACK_INDEX>();
    }

    // Claim personal ownership over the ZDO
    [[maybe_unused]] bool SetLocal() {
        return SetOwner(VH_ID);
    }

    // Clears the owner of this ZDO
    void Disown() {
        SetOwner(0);
    }

    // Set the owner of the ZDO
    [[maybe_unused]] bool SetOwner(OWNER_t owner) {
        // only if the owner has changed, then revise it
        if (this->Owner() != owner) {
            _SetOwner(owner);

            m_rev.ReviseOwner();
            return true;
        }
        return false;
    }

    // Set the owner of the ZDO without revising
    void _SetOwner(OWNER_t owner) {
        m_pack.Set<OWNER_PACK_INDEX>(ZDOID::EnsureUserIDIndex(owner));
    }



    [[nodiscard]] uint16_t GetOwnerRevision() const {
        return m_rev.GetOwnerRevision();
    }

    [[nodiscard]] uint32_t GetDataRevision() const {
        return m_rev.GetDataRevision();
    }



    [[nodiscard]] bool IsPersistent() const {
        return m_pack.Get<FLAGS_PACK_INDEX>() & (1 << MACHINE_Persistent);
    }

    [[nodiscard]] bool IsDistant() const {
        return m_pack.Get<FLAGS_PACK_INDEX>() & (1 << MACHINE_Distant);
    }

    [[nodiscard]] ObjectType GetType() const {
        return ObjectType((m_pack.Get<FLAGS_PACK_INDEX>() >> MACHINE_Type1) & 0b11);
    }



    [[nodiscard]] size_t GetTotalAlloc() {
        size_t size = 0;
        //for (auto&& pair : m_members) size += sizeof(Ord) + pair.second.GetTotalAlloc();
        return size;
    }
};
