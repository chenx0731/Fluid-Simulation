#include "Game/SPHKernels.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec3.hpp"

const float PI = 3.141592653589793f;

bool SPHKernel::m_isUsingCubicSpline = false;

float SPHKernel::Poly6(Vec3 const& r, float h)
{
	float lengthSquared = r.GetLengthSquared();
	float res = 0.0f;
	if (m_isUsingCubicSpline) {
		float length = r.GetLength();
		float q = length / h;
		if (q <= 0.5)
		{
			res = (8.0f / PI * Pow(h, 3)) * (6.0f * Pow(q, 3) - 6.0f * Pow(q, 2) + 1.0f);
		}
		else if (q < 2.f) res = (16.0f / (PI * Pow(h, 3))) * Pow(1 - q, 3);
	}
	else {
		if (lengthSquared < h * h) {
			float squaredDiff = h * h - lengthSquared;
			res = 315.0f * Pow(squaredDiff, 3);
			res /= (64.0f * PI * Pow(h, 9));
		}
	}
	return res;
}

Vec3 SPHKernel::SpikyGrad(Vec3 const& r, float h)
{
	Vec3 res = Vec3();
	//if ((float)r.GetLengthSquared() < h * h) {
	//	float length = (float)r.GetLength();
	//	float lengthDiff = h - length;
	//	res = r.GetNormalized();
	//	res *= -45.0 * Pow(lengthDiff, 2);
	//	res /= (PI * Pow(h, 6));
	//}
	if (m_isUsingCubicSpline) {
		float rl = (r.GetLength());
		float q = rl / h;
		if (rl > 1.0e-6f) {
			Vec3 gradq = (1.0f / (rl * h)) * r;
			if (q <= 0.5f) {
				res = (48.0f / (PI * Pow(h, 3)) * q * (3.0f * q - 2.0f)) * gradq;
			}
			else {
				float factor = 1.0f - q;
				res = (48.0f / (PI * Pow(h, 3)) * (-factor * factor)) * gradq;
			}
		}
	}
	else {
		if ((float)r.GetLengthSquared() < h * h) {
			float length = r.GetLength();
			float lengthDiff = h - length;
			res = r.GetNormalized();
			res *= (-45.0f * Pow(lengthDiff, 2));
			res /=(PI * Pow(h, 6));
		}
	}

	return res;
}
