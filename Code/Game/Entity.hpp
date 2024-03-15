#pragma once
#include "Game/Game.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/EulerAngles.hpp"

class Entity
{
public:
	Entity(Game* owner, Vec3 const& startPos);
	virtual ~Entity();

	virtual void Update() = 0;
	virtual void Render() const = 0;
	virtual void DebugRender() const;
	virtual void Die();
	
	Mat44 GetModelMatrix() const;

public:
	Game*		m_game = nullptr;
	Rgba8		m_color = Rgba8::WHITE;
	Vec3		m_position;
	Vec3		m_velocity;
	EulerAngles m_orientation;
	EulerAngles m_angularVelocity;
	
};