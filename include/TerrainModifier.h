#pragma once

class TerrainModifier {

public:
    enum class PaintType : std::uint8_t {
		Dirt,
		Cultivate,
		Paved,
		Reset, // use as the default for no modification
    };

};
