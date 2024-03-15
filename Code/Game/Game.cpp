#define UNUSED(x) (void)(x);
#include "Game/Game.hpp"
#include "Game/Entity.hpp"
#include "Game/Player.hpp"
#include "Game/Prop.hpp"
#include "Game/App.hpp"
#include "Game/FluidSolver.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Renderer/SimpleTriangleFont.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/DebugRenderSystem.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Core/Time.hpp"

Vertex_PCU vertices[] = {
	Vertex_PCU(Vec2(800.f, 600.f), Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f)),
	Vertex_PCU(Vec2(400.f, 200.f), Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f)),
	Vertex_PCU(Vec2(1200.f, 200.f), Rgba8(255, 255, 255, 255), Vec2(0.f, 0.f)),
};


Game::Game(App* owner)
{
	UNUSED(owner);

	m_clock = new Clock(Clock::GetSystemClock());
	m_fluidSolver = new FluidSolver();

	m_simCBO = g_theRenderer->CreateConstantBuffer(sizeof(SimulatorConstants));
	m_sortCBO = g_theRenderer->CreateConstantBuffer(sizeof(SortConstants));
	m_renderCBO = g_theRenderer->CreateConstantBuffer(sizeof(RenderConstants));
	m_lightCBO = g_theRenderer->CreateConstantBuffer(sizeof(FluidLightConstants));
	m_blurCBO = g_theRenderer->CreateConstantBuffer(sizeof(BlurConstants));
	
}

Game::~Game()
{
}

void Game::Startup()
{
	LoadAssets();
	
	Vec2 worldCenter(WORLD_CENTER_X, WORLD_CENTER_Y);

	m_player = new Player(this, Vec3(-2.45f, 15.6f, 2.79f));
	m_player->m_orientation.m_yawDegrees = 291.38f;
	m_player->m_orientation.m_pitchDegrees = 6.f;
	m_player->m_orientation.m_rollDegrees = 0.634f;
	m_player->m_camera.m_viewport = AABB2(Vec2(0.f, 0.f),
		Vec2((float)g_theRenderer->GetConfig().m_window->GetClientDimensions().x,
			(float)g_theRenderer->GetConfig().m_window->GetClientDimensions().y));
	
	
	SetCamera();

	m_fluidSolver->SetUpModel();

	m_shader = g_theRenderer->CreateShader("Data/Shaders/LinearDepthVisualizer");

}

void Game::Update()
{
	double updateStart = GetCurrentTimeSeconds();
	UpdateFromKeyboard();
	UpdateFromController();

	SetCamera();
	
	if (!m_isPaused)
	{
		UpdateEntities();

		DeleteGarbageEntities();
	}
	double updateTotal = GetCurrentTimeSeconds() - updateStart;
	//DebuggerPrintf(Stringf("Update: %.2lf\n", updateTotal * 1000).c_str());
	
}

void Game::Render() const
{
	//if (m_isAttractMode)
	//{
	//	RenderAttractMode();
	//	return;
	//}
	double start = GetCurrentTimeSeconds();
	g_theRenderer->ClearScreen(Rgba8::BLACK);
	//RenderGrid();
	RenderEntities();

	if (m_isDebug)
	{
		DebugRenderEntities();
	}
	
	RenderUI();

	if (m_isPaused)
	{
		RenderPaused();
	}
	
	DebugRenderWorld(m_player->m_camera);
	DebugRenderScreen(m_screenCamera);
	double total = GetCurrentTimeSeconds() - start;
	//DebuggerPrintf(Stringf("Render: %.2lfms\n", total * 1000).c_str());
}

void Game::Shutdown()
{
	delete m_fluidSolver;
	m_fluidSolver = nullptr;

	delete m_simCBO;
	m_simCBO = nullptr;

	delete m_sortCBO;
	m_sortCBO = nullptr;

	delete m_renderCBO;
	m_renderCBO = nullptr;

	delete m_lightCBO;
	m_lightCBO = nullptr;

	delete m_blurCBO;
	m_blurCBO = nullptr;
}

void Game::UpdateEntities()
{

	m_player->Update();
	
	m_fluidSolver->Update();
}

void Game::LoadAssets()
{
	//g_musicLibrary[MUSIC_WELCOME] = g_theAudio->CreateOrGetSound("Data/Audio/Click.mp3");

	g_textureLibrary[TEXTURE_SPHERE] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/TestUV.png");
	g_textureLibrary[TEXTURE_CUBE] = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Test_StbiFlippedAndOpenGL.png");

}

void Game::DeleteGarbageEntities()
{

}

void Game::KillAll()
{

}

void Game::UpdateFromKeyboard()
{
	m_lastCursorPos = m_nowCursorPos;
	m_nowCursorPos = g_theInput->GetCursorClientPosition();
	if (g_theInput->WasKeyJustPressed('P'))
	{
		m_isPaused = !m_isPaused;
	}
	//if (g_theInput->WasKeyJustPressed(' ') || g_theInput->WasKeyJustPressed('N'))
	//{
	//	if (m_isAttractMode)
	//	{
	//		EnterGame();
	//	}
	//		
	//}
	if (g_theInput->WasKeyJustPressed('O'))
	{
		m_isPaused = true;
		SetCamera();

		if (m_isAttractMode)
		{
			return;
		}

		UpdateEntities();

		DeleteGarbageEntities();

	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_F1))
	{
		m_isDebug = !m_isDebug;
	}
	if (g_theInput->IsKeyDown(KEYCODE_RIGHT_MOUSE)) {
		g_theGame->m_isAttractMode = false;
		g_theInput->SetCursorMode(true, true);
		if (!g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE)) {
			m_player->m_orientation.m_yawDegrees -= g_theInput->GetCursorClientDelta().x * 0.005f;
			m_player->m_orientation.m_pitchDegrees += g_theInput->GetCursorClientDelta().y * 0.005f;
		}
	}
	if (g_theInput->WasKeyJustReleased(KEYCODE_RIGHT_MOUSE)) {
		g_theGame->m_isAttractMode = true;
		g_theInput->SetCursorMode(false, false);
	}
	std::string msg = Stringf("Player position: %.2f, %.2f, %.2f                                        time = %.2f, fps = %.2f, scale = %.1f", 
		m_player->m_position.x, m_player->m_position.y, m_player->m_position.z, 
		m_clock->GetTotalSeconds(), 1.f / m_clock->GetDeltaSeconds(), m_clock->GetTimeScale());
	DebugAddMessage(msg, 0.f);
}

void Game::UpdateFromController()
{

}

void Game::RenderAttractMode() const
{
	g_theRenderer->BeginCamera(m_screenCamera);
	
	g_theRenderer->SetModelConstants();
	g_theRenderer->ClearScreen(Rgba8(100, 255, 255));
	g_theRenderer->DrawVertexArray(3, vertices);
	DebugDrawRing(Vec2(SCREEN_CAMERA_SIZE_X * 0.5f, SCREEN_CAMERA_SIZE_Y * 0.5),
		(150.f + 50.f * SinDegrees(100.f * m_clock->GetTotalSeconds())),
			(20.f + 5.f * CosDegrees(100.f * m_clock->GetTotalSeconds())),
				Rgba8(255, 150, 0));
	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderUI() const
{
	g_theRenderer->BeginCamera(m_screenCamera);

	m_fluidSolver->RenderScreenSpace();

	g_theRenderer->EndCamera(m_screenCamera);
}


void Game::RenderEntities() const
{
	g_theRenderer->BeginCamera(m_player->m_camera);

	//m_prop1->Render();

	//m_prop2->Render();

	//m_sphere->Render();

	m_fluidSolver->Render();

	g_theRenderer->EndCamera(m_player->m_camera);
}

void Game::RenderPaused() const
{
	g_theRenderer->BeginCamera(m_screenCamera);
	DrawSquare(m_screenCamera.m_bottomLeft, m_screenCamera.m_topRight, Rgba8(127, 127, 127, 127));
	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderGrid() const
{
	g_theRenderer->BeginCamera(m_player->m_camera);
	//g_theRenderer->SetModelConstants();
	//std::vector<Vertex_PCU> sphere;
	//AddVertsForSpere3D(sphere, Vec3(10.f, -5.f, 1.f), 1.f);
	//g_theRenderer->BindTexture(g_textureLibrary[TEXTURE_SPHERE]);
	//g_theRenderer->DrawVertexArray((int)sphere.size(), sphere.data());
	g_theRenderer->SetModelConstants();
	g_theRenderer->BindTexture(nullptr);
	std::vector<Vertex_PCU> grids;
	for (int xIndex = -50; xIndex <= 50; xIndex++) {
		float offset = 0.05f;
		Rgba8 color = Rgba8::GREY;
		if (xIndex % 5 == 0) {
			color = Rgba8::GREEN;
			offset *= 2;
		}
		Vec2 mins = Vec2(float(xIndex) - offset, -50.f);
		Vec2 maxs = Vec2(float(xIndex) + offset, 50.f);
		AddVertsForAABB2D(grids, AABB2(mins, maxs), color, Vec2(0.f, 0.f), Vec2(1.f, 1.f));
	}

	for (int yIndex = -50; yIndex <= 50; yIndex++) {
		float offset = 0.05f;
		Rgba8 color = Rgba8::GREY;
		if (yIndex % 5 == 0) {
			color = Rgba8::RED;
			offset *= 2;
		}
		Vec2 mins = Vec2(-50.f, float(yIndex) - offset);
		Vec2 maxs = Vec2(50.f, float(yIndex) + offset);
		AddVertsForAABB2D(grids, AABB2(mins, maxs), color, Vec2(0.f, 0.f), Vec2(1.f, 1.f));
	}
	g_theRenderer->DrawVertexArray((int)grids.size(), grids.data());
	g_theRenderer->EndCamera(m_player->m_camera);
}

void Game::DebugRenderEntities() const
{
	g_theRenderer->BeginCamera(m_player->m_camera);

	g_theRenderer->EndCamera(m_player->m_camera);
}

void Game::EnterGame()
{
	//SoundID testSound = g_theAudio->CreateOrGetSound("Data/Audio/Click.mp3");
	//PlayMusic(MUSIC_WELCOME);
	//g_theAudio->StartSound(testSound);
	Shutdown();
	Startup();
	m_isAttractMode = false;
	g_theInput->EndFrame();
}

void Game::SetCamera()
{
	//m_worldCamera.SetOrthoView(Vec2(-1.f, -1.f), Vec2(WORLD_CAMERA_SIZE_X, WORLD_CAMERA_SIZE_Y));
	m_player->m_camera.SetPerspectiveView(2.f, 60.f, 0.05f, 200.f);
	m_player->m_camera.SetRenderBasis(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
	m_screenCamera.SetOrthoView(Vec2(0.f, 0.f), Vec2(SCREEN_CAMERA_SIZE_X, SCREEN_CAMERA_SIZE_Y), 0.f, 2.f);
}

void Game::ReturnAttractModeVictory()
{
	ReturnAttractModeNormal();
}

void Game::ReturnAttractModeFailed()
{
	ReturnAttractModeNormal();

}

void Game::ReturnAttractModeNormal()
{
	//PlayMusic(MUSIC_WELCOME);
	m_isAttractMode = true;
}
