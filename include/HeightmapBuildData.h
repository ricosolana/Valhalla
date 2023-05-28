#pragma once

#include "VUtils.h"

#if VH_IS_ON(VH_ZONE_GENERATION)

#include "Biome.h"

class HMBuildData {
public:
	using Heights_t = std::vector<float>;
	using Mask_t = std::vector<Color>;

public:
	std::array<Biome, 4> m_cornerBiomes;

	Heights_t m_baseHeights;
	Mask_t m_baseMask;
};
#endif