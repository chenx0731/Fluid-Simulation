const double PI = 3.141592653589793;

double CubicSplineKernel(float3 r, double h)
{
	//double lengthSquared = asdouble(length(r));
	//float
	double res = 0.0;
	double length = length(r);
	double q = length / h;
	if (q <= 0.5)
	{
		res = (8.0 / PI * pow(h, 3)) * (6.0 * pow(q, 3) - 6.0 * pow(q, 2) + 1.0);
	}
	else if (q < 2)
	{
		res = (16.0 / (PI * pow(h, 3))) * pow(1 - q, 3);
	}
	return res;
}

float3 CubicSplineGradKernel(float3 r, double h)
{
	float3 res = float3(0.f);
	double length = length(r);
	double q = length / h;
	if (length > 1.0e-6)
	{
		float3 gradq = (1.0 / (length * h)) * r;
		if (q <= 0.5)
		{
			res = mul((48.0 / (PI * pow(h, 3)) * q * (3.0 * q - 2.0)), gradq);
		}
		else
		{
			double factor = 1.0 - q;
			res = mul(48.0 / ((PI * pow(h, 3)) * (-factor * factor)), gradq);
		}
	}
	return res;
}