#define UNUSED(x) (void)(x);
#include "Game/PositionBasedFluid.hpp"
#include "Game/Particle.hpp"
#include "Game/SPHKernels.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec3.hpp"

bool PositionBasedFluid::CalculateDensity(const int& particleIndex,
	const unsigned int& numFluidParticle,
	const float& particleRadius,
	const std::vector<Particle>& particles,
	const std::vector<unsigned int> neighbors,
	const float& density0,
	float& density)
{
	float res = 0.0f;
	UNUSED(density0);
	UNUSED(numFluidParticle);
	const Particle& pi = particles[particleIndex];
	res += pi.GetMass() * SPHKernel::Poly6(Vec3(), particleRadius);
	for (int neighborIndex = 0; neighborIndex < (int)neighbors.size(); neighborIndex++) {
		const Particle& pj = particles[neighbors[neighborIndex]];
		Vec3 r = pi.GetPosition() - pj.GetPosition();
		res += pj.GetMass() * SPHKernel::Poly6(r, particleRadius);
	}
	density = res;
	return true;
}

bool PositionBasedFluid::CalculateLagrangeMultiplier(const int& particleIndex,
	const unsigned int& numFluidParticle,
	const float& particleRadius,
	const std::vector<Particle>& particles,
	const std::vector<unsigned int> neighbors,
	const float& density0,
	const float& density,
	float& lambda)
{
	//double res = 0.0;
	float  eps = 1.0e-6f;
	UNUSED(numFluidParticle);
	float ci = density / density0 - 1.0f;
	if (ci < 0.0f)
		ci = 0.0f;
	if (ci == 0.0f) {
		lambda = 0.0f;
		return true;
	}
	float sum_gradj = 0.0f;
	Vec3 gradi = Vec3();
	const Particle& pi = particles[particleIndex];
	//double density0Squared = density0 * density0;
	//double inverseD02 = 1.0 / density0Squared;
	float inverseD0 = 1.0f / density0;
	for (int neighborIndex = 0; neighborIndex < (int)neighbors.size(); neighborIndex++) {
		const Particle& pj = particles[neighbors[neighborIndex]];
		Vec3 r = pi.GetPosition() - pj.GetPosition();
		Vec3 gradj = - SPHKernel::SpikyGrad(r, particleRadius) * static_cast<float>(inverseD0 * pj.GetMass());
		sum_gradj += gradj.GetLengthSquared() ;
		gradi += gradj;
	}
	sum_gradj += gradi.GetLengthSquared();
	lambda = -ci / (sum_gradj + eps);
	return true;
}

bool PositionBasedFluid::CalculateDeltaPos(const int& particleIndex,
	const unsigned int& numFluidParticle,
	const float& particleRadius,
	const std::vector<Particle>& particles,
	const std::vector<unsigned int> neighbors,
	const float& density0,
	const std::vector<float>& lambda,
	Vec3& pos)
{
	Vec3 deltaP = Vec3();
	const Particle& pi = particles[particleIndex];
	for (int neighborIndex = 0; neighborIndex < (int)neighbors.size(); neighborIndex++) {
		const Particle& pj = particles[neighbors[neighborIndex]];
		Vec3 r = pi.GetPosition() - pj.GetPosition();
		Vec3 gradi = SPHKernel::SpikyGrad(r, particleRadius) * (pj.GetMass());
		Vec3 q = r.GetNormalized() * 0.2f * ( particleRadius);
		//float Scorr = -0.1f * Pow((SPHKernel::Poly6(r, particleRadius) / SPHKernel::Poly6(Vec3(), particleRadius)), 4) ;
		if (neighbors[neighborIndex] < numFluidParticle)
			deltaP +=  (lambda[particleIndex] + lambda[neighbors[neighborIndex]] ) * gradi;
		else deltaP += (lambda[particleIndex]) * gradi; // add scorr to solid particles too, may remove it later
	}
	deltaP /= (density0);
	pos += deltaP;
	return true;
}
