#pragma once

#include "Vector.h"
#include <cstdint>
#include <optional>

namespace avledet::util {

    // TODO
    //  Class was manually copied piece by piece
    //  because it appears Valheim uses only small parts of it

    class FastNoise {
    public:
        enum class NoiseType
        {
            // Token: 0x04000165 RID: 357
            Value,
            // Token: 0x04000166 RID: 358
            ValueFractal,
            // Token: 0x04000167 RID: 359
            Perlin,
            // Token: 0x04000168 RID: 360
            PerlinFractal,
            // Token: 0x04000169 RID: 361
            Simplex,
            // Token: 0x0400016A RID: 362
            SimplexFractal,
            // Token: 0x0400016B RID: 363
            Cellular,
            // Token: 0x0400016C RID: 364
            WhiteNoise,
            // Token: 0x0400016D RID: 365
            Cubic,
            // Token: 0x0400016E RID: 366
            CubicFractal
        };

        enum class FractalType
        {
            NONE, // SIMILAR IN THIS CASE TO C# NULL

            // Token: 0x04000174 RID: 372
            FBM,
            // Token: 0x04000175 RID: 373
            Billow,
            // Token: 0x04000176 RID: 374
            RigidMulti
        };

        enum class CellularDistanceFunction
        {
            // Token: 0x04000178 RID: 376
            Euclidean,
            // Token: 0x04000179 RID: 377
            Manhattan,
            // Token: 0x0400017A RID: 378
            Natural
        };

        enum class CellularReturnType
        {
            // Token: 0x0400017C RID: 380
            CellValue,
            // Token: 0x0400017D RID: 381
            NoiseLookup,
            // Token: 0x0400017E RID: 382
            Distance,
            // Token: 0x0400017F RID: 383
            Distance2,
            // Token: 0x04000180 RID: 384
            Distance2Add,
            // Token: 0x04000181 RID: 385
            Distance2Sub,
            // Token: 0x04000182 RID: 386
            Distance2Mul,
            // Token: 0x04000183 RID: 387
            Distance2Div
        };

        using Float2 = avledet::util::CSU::Vector2<double>;

    public: 
        FastNoise(std::int32_t seed);

        void SetSeed(int seed);

        void SetNoiseType(NoiseType noiseType);

        void SetFractalOctaves(int octaves);

        void SetCellularDistanceFunction(CellularDistanceFunction cellularDistanceFunction);

        void SetCellularReturnType(CellularReturnType cellularReturnType);

        // COMPLICATED CODE:

        double GetSimplexFractal(double x, double y);

	    double GetCellular(double x, double y);

    private:
        inline static int FastRound(double f)
        {
            if (f < 0.0)
            {
                return (int)(f - 0.5);
            }
            return (int)(f + 0.5);
        }

    	void CalculateFractalBounding();

        inline static int Hash2D(int seed, int x, int y)
        {
            int num = seed ^ (1619 * x);
            num ^= 31337 * y;
            num = num * num * num * 60493;
            return (num >> 13) ^ num;
        }

        // COMPLICATED CODE:

	    double SingleCellular(double x, double y);

    private:
        std::int32_t m_seed = 1337;

        double m_frequency = 0.01;

        NoiseType m_noiseType = NoiseType::Simplex;

        int m_octaves = 3;

        double m_gain = 0.5;

        FractalType m_fractalType;

        double m_fractalBounding {};

        CellularDistanceFunction m_cellularDistanceFunction {};

        CellularReturnType m_cellularReturnType {};

        float m_cellularJitter = 0.45f;
    };

};
