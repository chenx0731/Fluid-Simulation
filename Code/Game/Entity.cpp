#include "Game/Entity.hpp"
#include "Engine/Math/Mat44.hpp"

Entity::Entity(Game* owner, Vec3 const& startPos)
{
	m_game = owner;
	m_position = startPos;
}

Entity::~Entity()
{
}

void Entity::DebugRender() const
{
}

void Entity::Die()
{
}

Mat44 Entity::GetModelMatrix() const
{
	Mat44 mat = m_orientation.GetMatrix_XFwd_YLeft_ZUp();
	mat.SetTranslation3D(m_position);
	return mat;
}

