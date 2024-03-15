#pragma once
#include "Game/Entity.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec3.hpp"

class Particle
{
public:
	Particle() = default;
	~Particle();

private:
	float	m_mass = 0.0f;
	float	m_invMass = 0.0f;
	Vec3	m_position;
	Vec3	m_velocity;
	Vec3	m_lastPosition;
	Vec3	m_oldPosition;
	Vec3	m_restPosition;
	Vec3	m_acceleration;

public:
	void	SetMass(const float& mass);
	void	SetPosition(const Vec3& position);
	void	SetVelocity(const Vec3& velocity);
	void	SetLastPosition(const Vec3& lastPosition);
	void	SetRestPosition(const Vec3& restPosition);
	void	SetOldPosition(const Vec3& oldPosition);
	void	SetAcceleration(const Vec3& acceleration);
	void	UpdateVelocityByPosition(const float& timeStep);

	float	GetMass() const { return m_mass; }
	float	GetInvMass() const { return m_invMass; }
	Vec3	GetPosition() const { return m_position; }
	Vec3	GetVelocity() const { return m_velocity; }
	Vec3	GetLastPosition() const { return m_lastPosition; }
	Vec3	GetOldPosition() const { return m_oldPosition; }
	Vec3	GetRestPosition() const { return m_restPosition; }
	Vec3	GetAcceleration() const { return m_acceleration; }
};