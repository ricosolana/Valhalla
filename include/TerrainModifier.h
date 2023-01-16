#pragma once

class TerrainModifier {

public:
    enum class PaintType : uint8_t {
		Dirt,
		Cultivate,
		Paved,
		Reset, // use as the default for no modification
    };

};
