#pragma once

#include "VUtils.h"
#include <functional>

#if AVL_IS_ON(AVL_DUNGEON_GENERATION)

    #include "DungeonRoom.h"
    #include "Prefab.h"

class Dungeon
{
  public:
    enum class Algorithm
    {
        Dungeon,
        CampGrid,
        CampRadial
    };

    struct DoorDef
    {
        Prefab::Reference m_prefab;
        std::string m_connection_type = "";
        float m_chance = 0;

        DoorDef(Prefab::Reference prefab, 
            std::string connection_type,
            float chance) : m_prefab(prefab), m_connection_type(connection_type), m_chance(chance) {

        }
    };

    //std::string m_name;

    //Prefab const *m_prefab = nullptr;
    Prefab::Reference m_prefab;

    Algorithm m_algorithm;

    int m_max_rooms = 3;

    unsigned int m_min_rooms = 20;

    int m_min_required_rooms;

    avledet::util::Set<std::string, ankerl::unordered_dense::string_hash> m_requiredRooms;

    bool m_alternative_functionality;

    avledet::util::Theme m_themes = avledet::util::Theme::Crypt;

    // Order is significant (polled with Seeded Random)
    std::vector<DoorDef> m_door_types;// Serialized

    float m_door_chance = 0.5f;

    float m_max_tilt = 10;

    float m_tile_width = 8;

    int m_grid_size = 4;

    float m_spawn_chance = 1;

    float m_camp_radius_min = 15;

    float m_camp_radius_max = 30;

    float m_min_altitude = 1;

    int m_perimeter_sections;

    float m_perimeter_buffer = 2;

    //bool m_useCustomInteriorTransform;

    Vector3f m_interior_position;// {0, 5000, 0} for dg/cave
    Vector3f m_original_position;// {0, 110, 30} and varies for dg/cave

    // Order is significant (polled with Seeded Random)
    std::vector<std::unique_ptr<Room const>> m_available_rooms;

  public:
    Dungeon(Prefab::Reference prefab) 
        : m_prefab(prefab) {

    }

    //std::unique_ptr<DungeonGenerator> Generate(const Vector3f& pos, const Quaternion& rot) const;
    //std::unique_ptr<DungeonGenerator> Generate(const Vector3f& pos, const Quaternion& rot, avledet::util::Hash seed) const;
    //
    //std::unique_ptr<DungeonGenerator> Generate(ZDO& zdo) const;

    std::string_view get_name() {
        return m_prefab.get().m_name;
    }
};
#endif