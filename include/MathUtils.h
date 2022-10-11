#pragma once

namespace MathUtils {

	/*
	 * Linear algebra methods
	 */

	float SqMagnitude(float x, float y);

	float Magnitude(float x, float y);

	float SqDistance(float x1, float y1, float x2, float y2);

	float Distance(float x1, float y1, float x2, float y2);

	float Clamp01(float value);

	float Lerp(float a, float b, float t);

	// Linear interpolation
	float LerpStep(float l, float h, float v);

	float SmoothStep(float p_Min, float p_Max, float p_X);

	double LerpStep(double l, double h, double v);




	// fucky methods

	// Fast inverse square root
	float FISQRT(float n);
}
