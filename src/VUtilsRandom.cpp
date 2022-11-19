#include <stdint.h>
#include <limits>
#include "VUtilsRandom.h"

namespace VUtils::Random {

	Seed_t m_seed;

	void SetSeed(int32_t param_1) {
		int32_t* seed = (int32_t*) &m_seed;

		seed[0] = param_1;

		int32_t mut = param_1 * 0x6c078965 + 1;
		seed[1] = mut;
		mut *= 0x6c078965 + 1;
		seed[2] = mut;
		seed[3] = mut * 0x6c078965 + 1;
	}

	Seed_t GetSeed() {
		return m_seed;
	}

	float Range(float minInclude, float maxExclude) {
		uint32_t* seed = (uint32_t*)&m_seed;
		uint32_t mut = (seed[0] << 11) ^ seed[0];
		mut = ((((seed[3] >> 11) ^ mut) >> 8) ^ seed[3]) ^ mut;

		// shift the bits
		seed[0] = seed[1];
		seed[1] = seed[2];
		seed[2] = seed[3];
		seed[3] = mut;

		float fVar4 = ((float) (uint64_t)(mut & 0x7fffff)) * 1.192093e-07f;
		return ((1.0f - fVar4) * maxExclude) + (fVar4 * minInclude);
	}



    // Ease-in-out function
    // t: [0, 1]
    // https://www.desmos.com/calculator/tgpfii21pt
    static double myfade(double t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); }
    static double mylerp(double t, double a, double b) { return a + t * (b - a); }
    static double mygrad(int hash, double x, double y) {
        int h = hash & 15;                      // CONVERT LO 4 BITS OF HASH CODE
        double u = h < 8 ? x : y,                 // INTO 12 GRADIENT DIRECTIONS.
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

    using uint = uint32_t;
    using ulonglong = uint64_t;
    using longlong = int64_t;
    using undefined = uint8_t;
    using undefined4 = uint32_t;

    /*
    undefined8 PerlinNoise(ulonglong bytesX, ulonglong bytesY)

    {
        uint uFloorY;
        uint uFloorX;
        longlong A;
        longlong B;
        ulonglong X;
        uint Y;

        // Ghidra seemed unable to ecompile the floats passed as params
        undefined4 in_XMM0_Dc;
        undefined4 in_XMM0_Dd;
        undefined auVar1[16];
        undefined auVar2[16];
        float fFadeInput1;
        float fFadeInput2;
        float fVar3;
        float fVar4;
        undefined4 in_XMM1_Dc;
        undefined4 in_XMM1_Dd;
        float fVar5;
        float fVar6;
        float y;
        float fVar7;
        float x;
        float fadeOutputY;

        y = (float)((uint)bytesY & 0x7fffffff);
        x = (float)((uint)bytesX & 0x7fffffff);
        uFloorY = (uint)y;
        Y = uFloorY & 0xff;
        uFloorX = (uint)x;
        X = (ulonglong)(uFloorX & 0xff);
        y -= (float)uFloorY;

        //A = (longlong)(int)(*(int*)(perm + X) + Y);
        //B = (longlong)(int)(*(int*)(perm + X + 1) + Y);
        A = (longlong)perm[X] + Y;
        B = (longlong)perm[X + 1] + y;

        x -= (float)uFloorX;

        //uFloorY = *(uint*)(perm + (longlong) *(int*)(perm + B + 1));
        uFloorY = perm[perm[B + 1]];
        fFadeInput1 = 1.0;
        if (x <= 1.0) {
            fFadeInput1 = x;
        }

        uFloorX = uFloorY & 0xf;
        fVar6 = x - 1.0;
        fFadeInput2 = 1.0;
        if (y <= 1.0) {
            fFadeInput2 = y;
        }
        fadeOutputY = ((fFadeInput1 * 6.0 - 15.0) * fFadeInput1 + 10.0) *
            fFadeInput1 * fFadeInput1 * fFadeInput1;

        // Another iteration?
        fFadeInput1 = y - 1.0;

        if (uFloorX < 8) {
            fVar5 = fVar6;
            if (uFloorX < 4) {

                // in_XMM.. are probably the actual float value in register
                // bytesY | fFadeInput1 (t)
                //
                auVar1 = CONCAT412(in_XMM1_Dd,
                    CONCAT48(in_XMM1_Dc,
                        bytesY & 0xffffffff00000000 | (ulonglong)(uint)fFadeInput1)) &
                    (undefined[16])0xffffffffffffffff;
            }
            else {
            LAB_18075f1a4:
                auVar1 = ZEXT416(0);
            }
        }
        else {
            fVar5 = fFadeInput1;

            // Intelligent compiler optimization to test whether number equals 12 or 14
            // the 'z' is treated as zero in all cases
            if ((uFloorX - 0xc & 0xfffffffd) != 0) goto LAB_18075f1a4;
            auVar1 = CONCAT412(in_XMM0_Dd,
                CONCAT48(in_XMM0_Dc, bytesX & 0xffffffff00000000 | (ulonglong)(uint)fVar6)) &
                (undefined[16])0xffffffffffffffff;
        }

        if ((uFloorY & 1) != 0) {
            fVar5 = (float)((uint)fVar5 ^ 0x80000000);
        }
        if ((uFloorY & 2) != 0) {
            auVar2 = auVar1 ^ (undefined[16])0x80000000;
            auVar1 = CONCAT412(SUB164(auVar2 >> 0x60, 0),
                CONCAT48(SUB164(auVar2 >> 0x40, 0),
                    CONCAT44(SUB164(auVar1 >> 0x20, 0), SUB164(auVar2, 0))));
        }

        //uFloorY = *(uint*)(&PermutFirst151 + (longlong) * (int*)(&PermutSecondAnd + A * 4) * 4);
        uFloorY = perm[perm[A + 1]];

        uFloorX = uFloorY & 0xf;
        if (uFloorX < 8) {
            fVar3 = fFadeInput1;
            fFadeInput1 = x;
            if (3 < uFloorX) {
            LAB_18075f1ed:
                fVar3 = 0.0;
            }
        }
        else {
            fVar3 = x;
            if ((uFloorX - 0xc & 0xfffffffd) != 0) goto LAB_18075f1ed;
        }
        if ((uFloorY & 1) != 0) {
            fFadeInput1 = (float)((uint)fFadeInput1 ^ 0x80000000);
        }
        if ((uFloorY & 2) != 0) {
            fVar3 = (float)((uint)fVar3 ^ 0x80000000);
        }
        //uFloorY = *(uint*)(&PermutFirst151 + (longlong) * (int*)(&PermutFirst151 + B * 4) * 4);
        uFloorY = perm[perm[B]];

        uFloorX = uFloorY & 0xf;
        if (uFloorX < 8) {
            fVar4 = y;
            if (3 < uFloorX) {
            LAB_18075f23e:
                fVar4 = 0.0;
            }
        }
        else {
            fVar4 = fVar6;
            fVar6 = y;
            if ((uFloorX - 0xc & 0xfffffffd) != 0) goto LAB_18075f23e;
        }
        if ((uFloorY & 1) != 0) {
            fVar6 = (float)((uint)fVar6 ^ 0x80000000);
        }
        if ((uFloorY & 2) != 0) {
            fVar4 = (float)((uint)fVar4 ^ 0x80000000);
        }

        //uFloorY = *(uint*)(&PermutFirst151 + (longlong) * (int*)(&PermutFirst151 + A * 4) * 4);
        uFloorY = perm[perm[A]];

        uFloorX = uFloorY & 0xf;
        if (uFloorX < 8) {
            fVar7 = y;
            y = x;
            if (uFloorX < 4) goto LAB_18075f287;
        }
        else {
            fVar7 = x;
            x = y;
            if ((uFloorX - 0xc & 0xfffffffd) == 0) goto LAB_18075f287;
        }
        fVar7 = 0.0;
        x = y;
    LAB_18075f287:
        if ((uFloorY & 1) != 0) {
            x = (float)((uint)x ^ 0x80000000);
        }
        if ((uFloorY & 2) != 0) {
            fVar7 = (float)((uint)fVar7 ^ 0x80000000);
        }
        x = ((fVar4 + fVar6) - (fVar7 + x)) * fadeOutputY + fVar7 + x;
        return SUB168(CONCAT124(SUB1612(auVar1 >> 0x20, 0),
            ((((SUB164(auVar1, 0) + fVar5) - (fVar3 + fFadeInput1)) * fadeOutputY +
                fVar3 + fFadeInput1) - x) *
            ((fFadeInput2 * 6.0 - 15.0) * fFadeInput2 + 10.0) *
            fFadeInput2 * fFadeInput2 * fFadeInput2 + x), 0);
    }*/


    
    // type is casted to a float, idk whether statically, probably (bytes interpreted in place)
    float PerlinNoise(float x, float y) {        
        uint32_t X = (int32_t)x & 0xFF;
        uint32_t Y = (int32_t)y & 0xFF;

        x -= (int32_t)x;                                // FIND RELATIVE X,Y,Z
        y -= (int32_t)y;                                // OF POINT IN CUBE.

        int A = p[X] + Y; // , AA = p[A] + Z, AB = p[A + 1] + Z,      // HASH COORDINATES OF
        int B = p[X + 1] + Y; // , BA = p[B] + Z, BB = p[B + 1] + Z;      // THE 8 CUBE CORNERS,

        int BB = p[p[B + 1]];
        int AB = p[p[A + 1]];
        int BA = p[p[B + 0]];
        int AA = p[p[A + 0]];

        //double u = fade(std::min(1.f, x));                                // COMPUTE FADE CURVES
        //double v = fade(std::min(1.f, y));                                // FOR EACH OF X,Y,
        double u = myfade(x);
        double v = myfade(y);

        auto gradBB = mygrad(BB, x - 1, y - 1);
        auto gradAB = mygrad(AB, x, y - 1);
        auto gradBA = mygrad(BA, x - 1, y);
        auto gradAA = mygrad(AA, x, y);

        // now lerp the grads
        // 2 x lerp, 1 y lerp

        float res = 
            mylerp(v,
                mylerp(u, gradAA, gradBA),
                mylerp(u, gradAB, gradBB)
            );

        // (fVar1 + 0.69) / 1.483

        res += .69f;
        res /= 1.483f;

        return res;

        // End of gradient fn, then Lerp
        //x = ((fVar4 + f_sub1X_eqY) - (fVar5 + x)) * fadeOutputX + fVar5 + x;
        // 
        // This is the x-faded lerp
        //x = lerp(fadeOutputX, fVar5 + x, fVar4 + f_sub1X_eqY);
        
        ////lerp()
        //
        //return 
        //    SUB16_8(
        //        CONCAT12_4(
        //            SUB16_12(auVar1 >> 0x20),
        //            ((((SUB16_4(auVar1) + fPostX) - (fVar3 + f_FadeInX_sub1Y)) * fadeOutputX
        //        + fVar3 + f_FadeInX_sub1Y) - x) * myfade(fFadeInput2) + x));




        //return lerp(v, lerp(u, grad(p[AA], x, y),           // AND ADD
        //                               grad(p[BA], x - 1, y)),      // BLENDED
        //                       lerp(u, grad(p[AB], x, y - 1),       // RESULTS
        //                               grad(p[BB], x - 1, y - 1))), // FROM  8
        //               lerp(v, lerp(u, grad(p[AA + 1], x, y),       // CORNERS
        //                               grad(p[BA + 1], x - 1, y)),  // OF CUBE
        //                       lerp(u, grad(p[AB + 1], x, y - 1),
        //                               grad(p[BB + 1], x - 1, y - 1)));

    }



}
