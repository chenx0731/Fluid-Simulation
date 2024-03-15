#include "Game/Prop.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/Clock.hpp"


Prop::Prop(Game* owner, Vec3 const& startPos) : Entity(owner, startPos)
{
	m_game = owner;
	m_position = startPos;
}

Prop::~Prop()
{
}

void Prop::Update()
{
	m_orientation.m_pitchDegrees += m_angularVelocity.m_pitchDegrees * m_game->m_clock->GetDeltaSeconds();
	m_orientation.m_yawDegrees	 += m_angularVelocity.m_yawDegrees	 * m_game->m_clock->GetDeltaSeconds();
	m_orientation.m_rollDegrees  += m_angularVelocity.m_rollDegrees  * m_game->m_clock->GetDeltaSeconds();
}

void Prop::Render() const
{
	g_theRenderer->SetModelConstants(GetModelMatrix(), m_color);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->BindTexture(m_texture);
	//TransformVertexArrayXY3D()
	g_theRenderer->DrawVertexArray((int)m_vertexes.size(), m_vertexes.data());
	g_theRenderer->BindTexture(nullptr);

}

