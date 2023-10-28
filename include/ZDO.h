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

class ZDO {
    // TODO are these friend classes safe?
    friend class IZDOManager; 
    friend class IPrefabManager;
    friend class VHTest;
    friend class IValhalla;

public:
    class Rev {
    private:
        // DataRevision: 0, OwnerRevision: 1
        //BitPack<uint32_t, 21, 32 - 21> m_pack;
        BitPack<uint32_t, 23, 32 - 23> m_pack;

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
    static constexpr unsigned int MACHINE_Persistent = 0;
    static constexpr unsigned int MACHINE_Distant = 1;
    static constexpr unsigned int MACHINE_Type1 = 2;
    static constexpr unsigned int MACHINE_Type2 = 3;

    static constexpr unsigned int NETWORK_Connection = 0;
    static constexpr unsigned int NETWORK_Float = 1;
    static constexpr unsigned int NETWORK_Vec3 = 2;
    static constexpr unsigned int NETWORK_Quat = 3;
    static constexpr unsigned int NETWORK_Int = 4;
    static constexpr unsigned int NETWORK_Long = 5;
    static constexpr unsigned int NETWORK_String = 6;
    static constexpr unsigned int NETWORK_ByteArray = 7;
    static constexpr unsigned int NETWORK_Persistent = 8;
    static constexpr unsigned int NETWORK_Distant = 9;
    static constexpr unsigned int NETWORK_Type1 = 10;
    static constexpr unsigned int NETWORK_Type2 = 11;
    static constexpr unsigned int NETWORK_Rotation = 12;


    struct hash {
        using is_transparent = void; // enable heterogeneous overloads
        using is_avalanching = void; // mark class as high quality avalanching hash

        [[nodiscard]] auto operator()(ZDOID const& id) const noexcept -> uint64_t {
            return ankerl::unordered_dense::hash<ZDOID>{}(id);
        }

        [[nodiscard]] auto operator()(ZDO const* v) const noexcept -> uint64_t {
            return ankerl::unordered_dense::hash<ZDOID>{}(v->GetID());
        }

        [[nodiscard]] auto operator()(std::unique_ptr<ZDO> const& v) const noexcept -> uint64_t {
            return ankerl::unordered_dense::hash<ZDOID>{}(v->GetID());
        }
    };

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
    using unsafe_value = ZDO*;
    using unsafe_optional = ZDO*;

    using container = ankerl::unordered_dense::segmented_set<std::unique_ptr<ZDO>, hash, std::equal_to<>>;
    using id_container = UNORDERED_SET_t<ZDOID, hash, std::equal_to<>>; // hetero hash?
    using ref_container = ankerl::unordered_dense::segmented_set<unsafe_value, hash, std::equal_to<>>;
    
    [[nodiscard]] static unsafe_value make_unsafe_value(container::iterator itr) {
        return itr->get();
    }

    [[nodiscard]] static unsafe_value make_unsafe_value(const container::value_type& itr) {
        return itr.get();
    }

    [[nodiscard]] static unsafe_value make_unsafe_value(const ref_container::value_type& v) {
        return const_cast<ZDO::unsafe_value&>(v);
    }

    [[nodiscard]] static unsafe_optional make_unsafe_optional(unsafe_value v) {
        return v;
    }

    [[nodiscard]] static unsafe_optional make_unsafe_optional(container::iterator itr) {
        return itr->get();
    }
    
    [[nodiscard]] static unsafe_optional make_unsafe_optional(const container::value_type& itr) {
        return itr.get();
    }

    static inline const auto unsafe_nullopt = nullptr;
    
private:
    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] static member_hash hash_to_xhash(HASH_t in) {
        return static_cast<member_hash>(in)
            ^ static_cast<member_hash>(ankerl::unordered_dense::hash<size_t>{}(VUtils::Traits::tuple_index_v<T, member_tuple>));
    }

    template<typename T>
        requires is_member_v<T>
    [[nodiscard]] static HASH_t xhash_to_hash(member_hash in) {
        return static_cast<HASH_t>(in)
            ^ static_cast<HASH_t>(ankerl::unordered_dense::hash<size_t>{}(VUtils::Traits::tuple_index_v<T, member_tuple>));
    }

    // Set the object by hash (Internal use only; does not revise ZDO on changes)
    //  Returns whether the previous value was modified
    //  Throws on type mismatch
    template<typename T>
        requires is_member<T>::value
    [[maybe_unused]] static bool _Set(HASH_t key, T value, member_map& members) {
        auto mut = hash_to_xhash<T>(key);

        auto&& insert = members.insert({ mut, 0.f });
        if (insert.second) {
            // Then officially assign
            insert.first->second = std::move(value);

            //this->m_pack.Set<data_t::

            //m_pack.Merge<2>(1 << member_denotion<T>::value);
            //m_pack.Merge<FLAGS_PACK_INDEX>(member_flag_v<T>);
            return true;
        }
        else {
            //assert(m_pack.Get<2>() & (1 << member_denotion<T>::value));
            //assert(m_pack.Get<FLAGS_PACK_INDEX>() & member_flag_v<T>);

            // else try modifying it ONLY if the member is same type

            // Modify type
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
                throw std::runtime_error("zdo member hash collision");
            }
        }
    }

    template<typename T> 
        requires is_member_v<T>
    [[maybe_unused]] bool _Set(HASH_t key, T value) {
        return _Set(key, std::move(value), ZDO_MEMBERS[GetID()]);
    }

    template<typename T>
        requires is_member_v<T>
    decltype(auto) static _TryWriteType(DataWriter& writer, member_map& members, bool network) {
        int count{};

        // HEY YOU FROM THE FUTURE!
        //  If this fails, just look at Valheim code in case they fixed it
        //  The issue is that ZDO-member save-type differs btw network/disk
        static_assert(VConstants::GAME == std::string_view("0.217.27"));
        
        // hopefully fixed by next patch
        if (network) {            
            const auto begin_mark = writer.Position();

            for (auto&& pair : members) {
                auto&& data = std::get_if<T>(&pair.second);
                if (data) {
                    // Count is only written
                    //  if type exists
                    if (!count) {
                        writer.Write((BYTE_t)count);
                    }

                    writer.Write(xhash_to_hash<T>(pair.first));
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
        else {
            // Count everything beforehand
            for (auto&& pair : members) {
                auto&& data = std::get_if<T>(&pair.second);
                if (data) {
                    count++;
                }
            }

            // new encoded type write requires int
            static_assert(std::is_same_v<decltype(count), int>);

            writer.WriteNumItems(count);

            for (auto&& pair : members) {
                auto&& data = std::get_if<T>(&pair.second);
                if (data) {
                    writer.Write(xhash_to_hash<T>(pair.first));
                    writer.Write(*data);
                }
            }
        }

        return count;
    }

    // Read a zdo_type from the DataStream
    template<typename T>
        requires is_member_v<T> //&& (std::same_as<CountType, char16_t> || std::same_as<CountType, uint8_t>)
    static void _TryReadType(DataReader& reader, member_map& members, int worldVersion) {
        int count{};

        // TODO look at this upon a patch change
        static_assert(std::string_view(VConstants::GAME) == std::string_view("0.217.27"));

        if (worldVersion > 0 && worldVersion < 31)
            count = reader.Read<char16_t>();
        else if (worldVersion == 0 || worldVersion < 33)
            count = reader.Read<uint8_t>(); // current network and older load
        else
            count = reader.ReadNumItems();

        // charcoal_kiln has 250 strings
        //  half of them are empty...
        //  "very efficient"; given all the optimizations..

        assert(count > 0 && count < 255); // unlikely to be larger; dungeons only

        for (int i = 0; i < count; i++) {
            // ...fuck
            // https://stackoverflow.com/questions/2934904/order-of-evaluation-in-c-function-parameters
            auto hash(reader.Read<HASH_t>());
            auto type(reader.Read<T>());
            _Set(hash, type, members);
        }
    }



    // TODO rename _Revise() ?
    void Revise() {
        this->m_rev.ReviseData();
    }

    

    void _SetPrefabHash(HASH_t hash) {
        this->m_prefabHash = hash;
    }

    // Set the owner of the ZDO without revising
    void _SetOwner(USER_ID_t owner) {
        ZDO_OWNERS[GetID()] = owner;
    }

    void _SetPosition(Vector3f pos) {
        this->m_pos = pos;
    }

    void _SetRotation(Vector3f rot) {
        this->m_rotation = rot;
    }

    void _SetRotation(Quaternion rot) {
        this->_SetRotation(rot.EulerAngles());
    }

    

private:
    static inline ankerl::unordered_dense::segmented_map<ZDOID, member_map> ZDO_MEMBERS;
    static inline ankerl::unordered_dense::segmented_map<ZDOID, ZDOConnectorData> ZDO_CONNECTORS; // Saved typed-connectors
    static inline ankerl::unordered_dense::segmented_map<ZDOID, ZDOConnectorTargeted> ZDO_TARGETED_CONNECTORS; // Current linked connectors
    static inline ankerl::unordered_dense::segmented_map<ZDOID, USER_ID_t> ZDO_OWNERS;

    // zdoid can be shrunk however, instead of using 8 + 4 bytes  (total 16 bytes; 4 bytes are extra padding), can be just 8 bytes (4 bytes for ID, 4 bytes for owner index)
    // because pair<K, V> includes padding, pair<zdoid, owner> uses the same memory as pair<zdoid, uint8_t>
    //static constexpr auto szz01311 = sizeof(decltype(ZDO_OWNERS)::value_type); // 24 bytes is a lot, unless zdoid can be aligned, and pair uses

    //static inline std::array<USER_ID_t, 
    //    //decltype(data_t::m_pack)::capacity_v<data_t::BIT_OWNER>
    //    64
    //> ZDO_OWNERS_INDEXES;

    //static constexpr auto OWNER_PACK_INDEX = 0;
    //static constexpr auto FLAGS_PACK_INDEX = 1;

    /*
    * 48 bytes currently;
    *   40 bytes: assuming mild ZDOID optimizations
    *       32 bytes: 8+12+4+4+4 = assuming encoded rotation
    *       30 bytes: assuming prefab-index
    *   38 bytes: assuming Valheim-equal optimizations
    *       30 bytes: 6+12+4+4+4 = assuming encoded rotation
    *       28 bytes: assuming prefab-index
    *   
    */

    ZDOID m_id;                                     // 16 bytes
    Vector3f m_pos;                                 // 12 bytes
    ZDO::Rev m_rev;                                 // 4 bytes (PADDING)
    Vector3f m_rotation;                            // 12 bytes
    HASH_t m_prefabHash{};                          // 4 bytes (PADDING)

public:
    ZDO(ZDOID id)
        : m_id(id)
    {}

    friend bool operator==(ZDOID const& lhs, ZDO const* rhs) noexcept {
        //assert((lhs != rhs.get()) == (lhs->GetID() != rhs->GetID()));
    
        return lhs == rhs->GetID();
    }

    friend bool operator==(ZDOID const& lhs, std::unique_ptr<ZDO> const& rhs) noexcept {
        return lhs == rhs->GetID();
    }

    // Apply changes to ZDOManager
    //  make this a lua-only method?
    //bool Apply() const;

#if VH_IS_ON(VH_LEGACY_WORLD_LOADING)
    // Load ZDO from disk
    void Load31Pre(DataReader& reader, int32_t version);
#endif //VH_LEGACY_WORLD_LOADING

    // Reads from a buffer using the new efficient format (version >= 31)
    //  version=0: Read according to the network deserialize format
    //  version>0: Read according to the file load format
    void Unpack(DataReader& reader, int32_t version);

    // Writes to a buffer using the new efficient format (version >= 31)
    //  If 'network' is true, write according to the network serialize format
    //  Otherwise write according to the file save format
    void Pack(DataWriter& writer, bool network) const;

    // TODO rename this to Remove (this has nearly the same functionality)
    // TODO add an extract that returns an optional (eliminate the T& out)
    // Erases and returns the value 
    template<typename T>
        requires is_member_v<T>
    bool Extract(HASH_t key, T& out) {
        //if (m_pack.Get<FLAGS_PACK_INDEX>() & member_flag_v<T>) {
            auto&& members_find = ZDO_MEMBERS.find(GetID());
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
            auto&& members_find = ZDO_MEMBERS.find(GetID());
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

    //member_variant_mono Extract(std::string key,)

    template<typename T>
        requires is_member_v<T>
    bool Extract(std::string_view key, T& out) {
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



    bool Extract(std::pair<HASH_t, HASH_t> key, ZDOID& out) {
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

    bool Extract(std::string_view key, ZDOID& out) {
        return Extract(VUtils::String::ToHashPair(key), out);
    }

    // Internal use
    //  Raw sets the connector with no revision
    bool _SetConnection(ZDOConnector::Type type, ZDOID zdoid) {
        auto&& insert = ZDO_TARGETED_CONNECTORS.insert({ GetID(),
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



    [[nodiscard]] ZDOID GetConnectionZDOID(ZDOConnector::Type type) const {
        auto&& find = ZDO_TARGETED_CONNECTORS.find(GetID());
        if (find != ZDO_TARGETED_CONNECTORS.end()) {
            if (find->second.m_type == type)
                return find->second.m_target;
        }
        return ZDOID::NONE;
    }


    [[nodiscard]] ZDOID GetID() const {
        return this->m_id;
    }

    [[nodiscard]] Vector3f GetPosition() const {
        return this->m_pos;
    }

    //void SetDataRevision(uint32_t dataRev) {
    //    m_data.get().m_rev.SetDataRevision(dataRev);
    //}
    //
    //void SetOwnerRevision(uint16_t ownerRev) {
    //    m_data.get().m_rev.SetOwnerRevision(ownerRev);
    //}



    Rev& GetRevision() {
        return this->m_rev;
    }

    // Set the position of the ZDO
    //  - Use this method 99.9% of the time when updating the ZDO's position
    //  - This will change and invalidate sectors if the new position is in a different zone than this ZDOs position
    void SetPosition(Vector3f pos);

    [[nodiscard]] ZoneID GetZone() const;

    [[nodiscard]] Quaternion GetRotation() const {
        return Quaternion::Euler(this->m_rotation);
    }

    void SetRotation(Quaternion rot) {
        auto&& euler = rot.EulerAngles();
        if (euler != this->m_rotation) {
            this->m_rotation = euler;
            this->Revise();
        }
    }
            
    [[nodiscard]] const Prefab& GetPrefab() const {
        return PrefabManager()->RequirePrefabByHash(this->m_prefabHash);
    }
    
    [[nodiscard]] HASH_t GetPrefabHash() const {
        return this->m_prefabHash;
    }

    void SetLocalScale(Vector3f scale, bool allowIdentity) {
        // if scaling along all axis VS scaling axis differently
        // this is just to save some memory
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
    [[nodiscard]] USER_ID_t Owner() const {
        // TODO optimize by checking owner bit
        auto&& find = ZDO_OWNERS.find(GetID());
        if (find != ZDO_OWNERS.end()) {
            return find->second;
        }
        return 0;
    }

    // Whether the ZDO is owned by a specific owner
    [[nodiscard]] bool IsOwner(USER_ID_t owner) const {
        return owner == this->Owner();
    }

    // Returns whether this server is the owner of the ZDO
    [[nodiscard]] bool IsLocal() const {
        return this->IsOwner(VH_ID);
    }

    // Whether the ZDO has an owner
    [[nodiscard]] bool HasOwner() const {
        return this->Owner() != 0;
        //return m_pack.Get<OWNER_PACK_INDEX>();
    }

    // Claim personal ownership over the ZDO
    bool SetLocal() {
        return this->SetOwner(VH_ID);
    }

    // Clears the owner of this ZDO
    void Disown() {
        this->SetOwner(0);
    }

    // Set the owner of the ZDO
    bool SetOwner(USER_ID_t owner) {
        // only if the owner has changed, then revise it
        if (this->Owner() != owner) {
            this->_SetOwner(owner);

            this->m_rev.ReviseOwner();
            return true;
        }
        return false;
    }



    [[nodiscard]] uint16_t GetOwnerRevision() const {
        return this->m_rev.GetOwnerRevision();
    }

    [[nodiscard]] uint32_t GetDataRevision() const {
        return this->m_rev.GetDataRevision();
    }



    [[nodiscard]] bool IsPersistent() const {
        return GetPrefab().IsPersistent();
    }

    [[nodiscard]] bool IsDistant() const {
        return GetPrefab().IsDistant();
    }

    [[nodiscard]] ObjectType GetType() const {
        return GetPrefab().GetObjectType();
    }



    [[nodiscard]] size_t GetTotalAlloc() const {
        size_t size = 0;

        auto&& find = ZDO_MEMBERS.find(GetID());
        if (find != ZDO_MEMBERS.end()) {
            for (auto&& member : find->second) {
                // TODO this only counts the compiled type size
                //  it does not include dynamically sized types like strings or arrays
                size += std::visit([](const auto& value) {
                    return sizeof(value);
                }, member.second);
            }
        }

        return size;
    }
};





