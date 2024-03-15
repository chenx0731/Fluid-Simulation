#pragma once
#include <vector>
struct Vec3;
class Particle;

class PositionBasedFluid
{
public:
	static bool CalculateDensity(
		const int& particleIndex,
		const unsigned int& numFluidParticle,
		const float& particleRadius,
		const std::vector<Particle>& particles,
		const std::vector<unsigned int> neighbors,
		const float& density0,
		float& density);

	static bool CalculateLagrangeMultiplier( 
		const int& particleIndex,
		const unsigned int& numFluidParticle,
		const float& particleRadius,
		const std::vector<Particle>& particles,
		const std::vector<unsigned int> neighbors,
		const float& density0,
		const float& density,
		float& lambda
	);

	static bool CalculateDeltaPos(
		const int& particleIndex,
		const unsigned int& numFluidParticle,
		const float& particleRadius,
		const std::vector<Particle>& particles,
		const std::vector<unsigned int> neighbors,
		const float& density0,
		const std::vector<float>& lambda,
		Vec3& pos
	);

};