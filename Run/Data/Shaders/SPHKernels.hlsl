static const float PI = 3.141592653589793f;


float Poly6(float3 r, float h, bool isUsingCubicSpline)
{
	float length_squared = length(r) * length(r);
	float res = 0.0;
	if (isUsingCubicSpline) {
		float Length = length(r);
		float q = Length / h;
		if (q <= 0.5)
		{
			res = (8.0 / PI * pow(h, 3)) * (6.0 * pow(q, 3) - 6.0 * pow(q, 2) + 1.0);
		}
		else if (q < 2) res = (16.0 / (PI * pow(h, 3))) * pow(1 - q, 3);
	}
	else {
		if (length_squared < h * h) {
			float squaredDiff = h * h - length_squared;
			res = 315.0 * pow(squaredDiff, 3);
			res /= (64.0 * PI * pow(h, 9));
		}
	}
	return res;
}

float3 SpikyGrad(float3 r, float h, bool isUsingCubicSpline)
{
	float3 res = 0.f.xxx;
	float lengthSquared = length(r) * length(r);
	if (isUsingCubicSpline) {
		float rl = length(r);
		float q = rl / h;
		if (rl > 1.0e-6) {
			float3 gradq = (1.0 / (rl * h)) * r;
			if (q <= 0.5) {
				res = (48.0 / (PI * pow(h, 3)) * q * (3.0 * q - 2.0)) * gradq;
			}
			else {
				float factor = 1.0 - q;
				res = (48.0 / (PI * pow(h, 3)) * (-factor * factor)) * gradq;
			}
		}
	}
	else {
		if (lengthSquared < h * h) {
			float Length = length(r);
			float lengthDiff = h - Length;
			if (length(r) != 0.f)
				res = normalize(r);
			res *= (-45.0 * pow(lengthDiff, 2));
			res /= ((float)PI * pow(h, 6));
		}
	}

	return res;
}
