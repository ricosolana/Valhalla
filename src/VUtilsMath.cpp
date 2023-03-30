#include <cmath>
#include <stdexcept>

#include "VUtilsMath.h"
#include "Vector.h"
#include "VUtils.h"
#include "VUtilsMathf.h"

namespace VUtils::Math {
    float SqMagnitude(float x, float y) {
        return x * x + y * y;
    }

    float Magnitude(float x, float y) {
        return sqrt(SqMagnitude(x, y));
    }

    float SqDistance(float x1, float y1, float x2, float y2) {
        return SqMagnitude(x1 - x2, y1 - y2);
    }

    float Distance(float x1, float y1, float x2, float y2) {
        return std::sqrtf(SqDistance(x1, y1, x2, y2));
    }




    float SqMagnitude(float x, float y, float z) {
        return x * x + y * y + z * z;
    }

    float Magnitude(float x, float y, float z) {
        return sqrt(SqMagnitude(x, y, z));
    }

    float SqDistance(float x1, float y1, float z1, float x2, float y2, float z2) {
        return SqMagnitude(x1 - x2, y1 - y2, z1 - z2);
    }

    float Distance(float x1, float y1, float z1, float x2, float y2, float z2) {
        return std::sqrtf(SqDistance(x1, y1, z1, x2, y2, z2));
    }







    float Clamp(float value, float min, float max) {
        return std::min(std::max(value, min), max);
    }



    float LerpStep(float l, float h, float v) {
        return Mathf::Clamp01((v - l) / (h - l));
    }

    float SmoothStep(float p_Min, float p_Max, float p_X) {
        float num = Mathf::Clamp01((p_X - p_Min) / (p_Max - p_Min));
        return num * num * (3.f - 2.f * num);
    }

    double LerpStep(double l, double h, double v) {
        return Mathf::Clamp01((v - l) / (h - l));
    }






    float Fbm(const Vector3f &p, int octaves, float lacunarity, float gain) {
        return Fbm(Vector2f(p.x, p.z), octaves, lacunarity, gain);
    }

    float FbmMaxValue(int octaves, float gain) {
        float num = 0;
        float num2 = 1;
        for (int i = 0; i < octaves; i++)
        {
            num += num2;
            num2 *= gain;
        }
        return num;
    }

    float Fbm(const Vector2f &p, int octaves, float lacunarity, float gain) {
        float num = 0;
        float num2 = 1;
        Vector2f vector = p;
        for (int i = 0; i < octaves; i++)
        {
            num += num2 * VUtils::Math::PerlinNoise(vector.x, vector.y);
            num2 *= gain;
            vector *= lacunarity;
        }
        return num;
    }






    float FISQRT(float number) {
        long i;
        float x2, y;
        const float threehalfs = 1.5F;

        x2 = number * 0.5F;
        y = number;
        i = *(long*)&y;                       // evil floating point bit level hacking
        i = 0x5f3759df - (i >> 1);               // what the fuck? 
        y = *(float*)&i;
        y = y * (threehalfs - (x2 * y * y));   // 1st iteration
        y = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

        return y;
    }










    // Ease-in-out function
    // t: [0, 1]
    // https://www.desmos.com/calculator/tgpfii21pt
    static double myfade(float t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }
    static double mylerp(float t, float a, float b) { return a + t * (b - a); }
    static double mygrad(int hash, float x, float y) {
        int h = hash & 15;                            // CONVERT LO 4 BITS OF HASH CODE
        float u = h < 8 ? x : y,                    // INTO 12 GRADIENT DIRECTIONS.
            v = h < 4 ? y : h == 12 || h == 14 ? x : 0;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    static BYTE_t p[] = { 151,160,137,91,90,15,
        131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
        190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
        88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
        77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
        102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
        135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
        5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
        223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
        129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
        251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
        49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
        // 2nd copy
        151,160,137,91,90,15,
        131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
        190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
        88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
        77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
        102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
        135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
        5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
        223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
        129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
        251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
        49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
        138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
    };

    // type is casted to a float, idk whether statically, probably (bytes interpreted in place)
    float PerlinNoise(float x, float y) {
        x = fabs(x);
        y = fabs(y);

        int X = (int)x & 0xFF;
        int Y = (int)y & 0xFF;

        x -= (int)x;
        y -= (int)y;

        int A = p[X] + Y;
        int B = p[X + 1] + Y;

        int BB = p[p[B + 1]];
        int AB = p[p[A + 1]];
        int BA = p[p[B + 0]];
        int AA = p[p[A + 0]];

        double u = myfade(x);
        double v = myfade(y);

        auto gradBB = mygrad(BB, x - 1, y - 1);
        auto gradAB = mygrad(AB, x, y - 1);
        auto gradBA = mygrad(BA, x - 1, y);
        auto gradAA = mygrad(AA, x, y);

        float res =
            mylerp(v,
                mylerp(u, gradAA, gradBA),
                mylerp(u, gradAB, gradBB)
            );

        return (res + .69f) / 1.483f;
    }


    float YawFromDirection(const Vector3f &dir) {
        float num = std::atan2(dir.x, dir.z);
        return FixDegAngle(57.29578f * num);
    }

    float FixDegAngle(float p_Angle) {
        while (p_Angle >= 360)
            p_Angle -= 360;

        while (p_Angle < 0)
            p_Angle += 360;

        return p_Angle;
    }



    bool Between(float i, float a, float b) {
        assert(a <= b);

        return i >= a && i <= b;
    }

}