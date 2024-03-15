#pragma once
#include "Game/Entity.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Rgba8.hpp"
#include <vector>

class Texture;

class Prop : public Entity
{
public:
	Prop(Game* owner, Vec3 const& startPos);
	~Prop();
	virtual void Update() override;
	virtual void Render() const override;

public:
	std::vector<Vertex_PCU> m_vertexes;
	Texture*				m_texture;
};