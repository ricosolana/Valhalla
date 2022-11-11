#include <cmath>
#include <stdexcept>

#include "VUtilsMath.h"
#include "Vector.h"

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
		return SqDistance(x1, y1, x2, y2);
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
		return SqDistance(x1, y1, z1, x2, y2, z2);
	}







	float Clamp(float value, float min, float max) {
		return std::min(std::max(value, min), max);
	}

	float Clamp01(float value)
	{
		bool flag = value < 0.f;
		float result;
		if (flag)
		{
			result = 0.f;
		}
		else
		{
			bool flag2 = value > 1.f;
			if (flag2)
			{
				result = 1.f;
			}
			else
			{
				result = value;
			}
		}
		return result;
	}

	float Lerp(float a, float b, float t)
	{
		return a + (b - a) * Clamp01(t);
	}


	float LerpStep(float l, float h, float v)
	{
		return Clamp01((v - l) / (h - l));
	}

	float SmoothStep(float p_Min, float p_Max, float p_X)
	{
		float num = Clamp01((p_X - p_Min) / (p_Max - p_Min));
		return num * num * (3.f - 2.f * num);
	}

	double LerpStep(double l, double h, double v)
	{
		return Clamp01((v - l) / (h - l));
	}






	float Fbm(const Vector3 &p, int octaves, float lacunarity, float gain) {
		return Fbm(Vector2(p.x, p.z), octaves, lacunarity, gain);
	}

	float FbmMaxValue(int octaves, float gain)
	{
		float num = 0;
		float num2 = 1;
		for (int i = 0; i < octaves; i++)
		{
			num += num2;
			num2 *= gain;
		}
		return num;
	}

	float Fbm(const Vector2 &p, int octaves, float lacunarity, float gain)
	{
		throw std::runtime_error("Not implemented");
		float num = 0;
		float num2 = 1;
		Vector2 vector = p;
		for (int i = 0; i < octaves; i++)
		{
			//num += num2 * Mathf.PerlinNoise(vector.x, vector.y);
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
        //	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

        return y;
    }
}