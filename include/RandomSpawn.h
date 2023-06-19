#pragma once

#include <vector>
#include "Prefab.h"

struct RandomSpawn {
	float m_chance;
	std::vector<Prefab::Instance> m_pieces;
};
