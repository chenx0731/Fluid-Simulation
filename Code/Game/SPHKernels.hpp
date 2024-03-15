#pragma once
struct Vec3;

class SPHKernel
{
public:
	// calculate density
	static float Poly6(Vec3 const& r, float h);
	// calculate density gradient
	static Vec3 SpikyGrad(Vec3 const& r, float h);

public:
	static bool m_isUsingCubicSpline;
};