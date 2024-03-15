#pragma once
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/Mat44.hpp"

class App;
class Entity;
class Player;
class Prop;
class Clock;
class Shader;
class FluidSolver;
class ConstantBuffer;


struct SimulatorConstants
{
	float m_deltaSeconds;
	float m_gravity;
	float m_particleRadius;
	float m_kernelRadius;

	float m_density0;
	int   m_numOfFluidParticles;
	int	  m_numOfParticles;
	bool  m_isUsingCubicSpline;

	float m_viscosity;
	Vec3  m_externalForce = Vec3();

	float m_scorrK;
	float m_scorrQ;
	float m_padding0;
	float m_padding1;
};
static const int k_simConstantsSlot = 0;

struct RenderConstants
{
	Vec3 m_eyePos;
	float m_sphereRadius;
	Vec4 m_eyeUp;
	Vec4 m_eyeLeft;
	Vec2 m_depthDimenstion;
	float padding0;
	float padding1;
	float m_waterColor[4];
};
static const int k_renderConstantsSlot = 4;

struct SortConstants
{
	int m_groupWidth;
	int m_groupHeight;
	int m_stepIndex;
	int m_numOfEntries = 0;
};
static const int k_sortConstantsSlot = 5;

struct BlurConstants
{
	float FilterRadius;
	Vec2 BlurDirt;
	float BlurScale;

	float BlurDepthFalloff;
	Vec3 Padding0;

	int	ImageWidth;
	int	ImageHeight;
	float SigmaD;
	float SigmaR;
};
static const int k_blurConstantsSlot = 6;

struct FluidLightConstants
{
	Vec3 m_lightDirt;
	float m_shininess;

	Vec3 m_lightPos;
	float m_fresPower;

	float m_fresScale;
	float m_fresBias;
	float m_lightIntensity;
	float m_specularIntensity;

	Mat44 m_InvSkyboxModelMat;

	Vec4  m_ambientIntensity;
};
static const int k_fluidLightConstantsSlot = 1;

class Game
{
public:
	Game( App* owner);
	~Game();
	void Startup();
	void Update();
	void Render() const;
	void Shutdown();

	void ReturnAttractModeVictory();
	void ReturnAttractModeFailed();
	void ReturnAttractModeNormal();
private:

	void LoadAssets();
	
	void DeleteGarbageEntities();
	void KillAll();

	void UpdateEntities();
	void UpdateFromKeyboard();
	void UpdateFromController();

	void RenderAttractMode() const;
	void RenderUI() const;
	void RenderEntities() const;
	void RenderPaused() const;
	void DebugRenderEntities() const;
	void RenderGrid() const;

	void EnterGame();
	void SetCamera();
	
public:

	Camera		m_worldCamera;
	Camera		m_screenCamera;

	bool		m_isDebug = false;
	bool		m_isPaused = false;
	bool		m_isShaking = false;
	bool		m_isAttractMode = true;
	bool		m_isOver = false;

	float		m_hangingTime = 0.f;
	int			m_current_level = 0;
	float		m_shakingTime = 0.f;

	Player*		m_player;
	Prop*		m_prop1;
	Prop*		m_prop2;
	Prop*		m_sphere;

	Vec2		m_nowCursorPos;
	Vec2		m_lastCursorPos;

	Clock*		m_clock;
	FluidSolver* m_fluidSolver = nullptr;
	Shader*		m_shader = nullptr;

	ConstantBuffer* m_simCBO = nullptr;
	ConstantBuffer* m_sortCBO = nullptr;
	ConstantBuffer* m_renderCBO = nullptr;
	ConstantBuffer* m_lightCBO = nullptr;
	ConstantBuffer* m_blurCBO = nullptr;
};