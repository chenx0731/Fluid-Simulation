#include "Game/Particle.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/Clock.hpp"

Particle::~Particle()
{
}

void Particle::SetMass(const float& mass)
{
	m_mass = mass;
	m_invMass = (mass != 0.0f) ? (1.0f / mass) : 0.0f;
}

void Particle::SetPosition(const Vec3& position)
{
	m_position = position;
}

void Particle::SetVelocity(const Vec3& velocity)
{
	m_velocity = velocity;
}

void Particle::SetLastPosition(const Vec3& lastPosition)
{
	m_lastPosition = lastPosition;
}

void Particle::SetRestPosition(const Vec3& restPosition)
{
	m_restPosition = restPosition;
}

void Particle::SetOldPosition(const Vec3& oldPosition)
{
	m_oldPosition = oldPosition;
}

void Particle::SetAcceleration(const Vec3& acceleration)
{
	m_acceleration = acceleration;
}

void Particle::UpdateVelocityByPosition(const float& timeStep)
{
	if (m_mass != 0.0f) {
		m_velocity = (m_position - m_oldPosition) / timeStep;
	}
}
