#include "Game/Player.hpp"
#include "Game/GameCommon.hpp"
#include "Game/FluidSolver.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Math/Mat44.hpp"

Player::Player(Game* owner, Vec3 const& startPos) : Entity(owner, startPos)
{
	m_game = owner;
	m_position = startPos;
	m_camera.SetPerspectiveView(2.f, 60.f, 0.05f, 200.f);
	m_camera.SetRenderBasis(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
}

Player::~Player()
{
}

void Player::Update()
{
	Vec3 x_forward;
	Vec3 y_left;
	Vec3 z_up;
	m_orientation.m_yawDegrees -= g_theInput->GetCursorClientDelta().x * 0.5f;
	m_orientation.m_pitchDegrees += g_theInput->GetCursorClientDelta().y * 0.5f;
	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.f, 85.f);

	m_orientation.GetVectors_XFwd_YLeft_ZUp(x_forward, y_left, z_up);

	XboxController tempController = g_theInput->GetController(0);


	if (g_theInput->IsKeyDown('S') || tempController.GetLeftStick().GetPosition().y < 0) {
		//m_position.x -= m_game->m_clock->GetDeltaSeconds() * 2.f;
		m_position -= m_game->m_clock->GetDeltaSeconds() * m_cameraSpeed * x_forward;
	}

	if (g_theInput->IsKeyDown('W') || tempController.GetLeftStick().GetPosition().y > 0) {
		//m_position.x += m_game->m_clock->GetDeltaSeconds() * 2.f;
		m_position += m_game->m_clock->GetDeltaSeconds() * m_cameraSpeed * x_forward;
	}
	if (g_theInput->IsKeyDown('Q')) {
		m_game->m_fluidSolver->m_externalForce = Vec3(5.f, 0.f, 0.f);
	}
	if (g_theInput->WasKeyJustReleased('Q')) {
		m_game->m_fluidSolver->m_externalForce = Vec3(0.f, 0.f, 0.f);
	}
	if (g_theInput->IsKeyDown('E')) {
		m_game->m_fluidSolver->m_externalForce = Vec3(-5.f, 0.f, 0.f);
	}
	if (g_theInput->WasKeyJustReleased('E')) {
		m_game->m_fluidSolver->m_externalForce = Vec3(0.f, 0.f, 0.f);
	}
	if (g_theInput->IsKeyDown('A') || tempController.GetLeftStick().GetPosition().x < 0) {
		//m_position.y += m_game->m_clock->GetDeltaSeconds() * 2.f;
		m_position += m_game->m_clock->GetDeltaSeconds() * m_cameraSpeed * y_left;
	}
	if (g_theInput->IsKeyDown('D') || tempController.GetLeftStick().GetPosition().x > 0) {
		//m_position.y -= m_game->m_clock->GetDeltaSeconds() * 2.f;
		m_position -= m_game->m_clock->GetDeltaSeconds() * m_cameraSpeed * y_left;
	}
	if (g_theInput->IsKeyDown('Z') || tempController.GetButton(XBOX_BUTTON_L_SHOULDER).IsKeyDown()) {
		//m_position.y += m_game->m_clock->GetDeltaSeconds() * 2.f;
		m_position += m_game->m_clock->GetDeltaSeconds() * m_cameraSpeed * Vec3(0.f, 0.f, 1.f);
	}
	if (g_theInput->IsKeyDown('C') || tempController.GetButton(XBOX_BUTTON_R_SHOULDER).IsKeyDown()) {

		m_position -= m_game->m_clock->GetDeltaSeconds() * m_cameraSpeed * Vec3(0.f, 0.f, 1.f);
	}
	if (g_theInput->IsKeyDown('H') || tempController.GetButton(XBOX_BUTTON_START).IsKeyDown()) {

		m_position = Vec3(0.f, 0.f, 0.f);
		m_orientation.m_yawDegrees = 0.f;
		m_orientation.m_pitchDegrees = 0.f;
		m_orientation.m_rollDegrees = 0.f;
	}
	if (g_theInput->IsKeyDown(KEYCODE_SHIFT) || tempController.GetButton(XBOX_BUTTON_A).IsKeyDown()) {
		m_cameraSpeed = 20.f;
	}
	if (g_theInput->WasKeyJustReleased(KEYCODE_SHIFT) || tempController.GetButton(XBOX_BUTTON_A).WasKeyJustReleased()) {
		m_cameraSpeed = 2.f;
	}
	if (g_theInput->IsKeyDown(KEYCODE_UPARROW) || tempController.GetRightStick().GetPosition().y > 0) {
		m_orientation.m_pitchDegrees -= m_game->m_clock->GetDeltaSeconds() * 40.f;
	}
	if (g_theInput->IsKeyDown(KEYCODE_DOWNARROW) || tempController.GetRightStick().GetPosition().y < 0) {
		m_orientation.m_pitchDegrees += m_game->m_clock->GetDeltaSeconds() * 40.f;
	}
	if (g_theInput->IsKeyDown(KEYCODE_RIGHTARROW) || tempController.GetRightStick().GetPosition().x > 0) {
		m_orientation.m_yawDegrees -= m_game->m_clock->GetDeltaSeconds() * 40.f;
	}
	if (g_theInput->IsKeyDown(KEYCODE_LEFTARROW) || tempController.GetRightStick().GetPosition().x < 0) {
		m_orientation.m_yawDegrees += m_game->m_clock->GetDeltaSeconds() * 40.f;
	}
	m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.f, 45.f);
	m_camera.SetTransform(m_position, m_orientation);
}

void Player::Render() const
{
}

