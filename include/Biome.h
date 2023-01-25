#pragma once

enum class Biome : uint16_t {
    None = 0,
    Meadows = 1 << 0,
    Swamp = 1 << 1,
    Mountain = 1 << 2,
    BlackForest = 1 << 3,
    Plains = 1 << 4,
    AshLands = 1 << 5,
    DeepNorth = 1 << 6,
    Ocean = 1 << 8,
    Mistlands = 1 << 9,
    BiomesMax // DO NOT USE
};

enum class BiomeArea : uint8_t {
    None = 0,
    Edge = 1 << 0,
    Median = 1 << 1,
    Everything = Edge | Median,
};
