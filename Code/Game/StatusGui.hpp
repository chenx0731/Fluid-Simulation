#pragma once
#include "Engine/UI/ImGuiBase.hpp"

class Game;

class StatusGui : public ImGuiBase
{
public:
	StatusGui(Game* game, ImGuiBaseConfig config);
	void Update() override;

public:
	Game* m_game;
};

