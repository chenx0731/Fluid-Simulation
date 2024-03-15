#pragma once
#include "Game/Entity.hpp"
#include "Game/Particle.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Renderer/Camera.hpp"

class Clock;
class VertexBuffer;
class IndexBuffer;
class ConstantBuffer;
class StructuredBuffer;
class NeighborSearch;
class Shader;
class Texture;

struct ID3D11Buffer;

enum ScreenViews
{
	SV_NONE = -1,
	SV_DEPTH,
	SV_BLURRED_DEPTH,
	SV_THICKNESS,
	SV_BLURRED_THICKNESS,
	SV_NORMAL_MAP,
	SV_FINAL_MERGED,
	SV_BACKGROUND,
};

class FluidSolver
{
public:
	FluidSolver();
	~FluidSolver();

	void SetUpModel();
	void ClearModel();
	void ResetSimulation();
	void ResetParticles(const int& size);
	void SetParticleRadius(const float& radius);

	void CreateComputeShaders();
	void CreateShaders();
	void CreateTextures();
	
	void InitMasses(const int& size);

	void AddGravity();
	void SortHashMap();
	void ProjectConstraint();
	void ApplyViscosity(int particleIndex);

	void Update();

	void Render() const;
	void RenderScreenSpace() const;
	void RenderSurfaceInfoFromLight() const;
	void RenderSandPlane() const;
	void RenderSkybox() const;
	void SetSimulatorCBO() const;
	void SetRenderCBO() const;
	void SetFluidLightCBO() const;
	void SetSandLightCBO() const;
	void SetBlurCBO(Vec2 dir, float sigmaD = 7.f) const;
	void ApplyBilateralFilter() const;

	Mat44 GetSkyboxMatrix() const;
public:
	float					m_density0;
	float					m_viscosity;
	float					m_sphRadius;
	float					m_particleRadius;
	float					m_rigidParticleRadius = 0.05f;
	unsigned int			m_maxIter;
	std::vector<Particle>	m_particles;
	std::vector<float>		m_density;
	std::vector<float>		m_lambda;
	std::vector<Vec3>		m_deltaPos;

	float					m_scorrK = -0.004f;
	float					m_scorrQ = 0.1f;

	Vec3					m_externalForce = Vec3();

	int						m_numOfParticles;
	int						m_fluidNumOfParticles;
	int						m_numOfThreads = 256;

	std::vector<Vertex_PCU> m_spheres;
	VertexBuffer*			m_gpuMesh;

	int						m_vertexCount;
	NeighborSearch*			m_neighborSearch = nullptr;
	
	Clock*					m_clock = nullptr;
	float					m_timeStep = 0.01f;

	bool					m_isApplyingPressure = true;
	bool					m_isApplyingViscosity = true;
	bool					m_isApplyingVorticity = true;
	bool					m_isUsingGPU = true;

	ScreenViews				m_currentView = SV_FINAL_MERGED;

	ScreenViews				m_miniDepth = SV_NONE;
	ScreenViews				m_miniThickness = SV_NONE;
	ScreenViews				m_miniNormal = SV_NONE;

	Shader*					m_applyGravityCS = nullptr;
	Shader*					m_spatialHashCS = nullptr;
	Shader*					m_sortCS = nullptr;
	Shader*					m_groupShareSortCS = nullptr;
	Shader*					m_computeOffsetCS = nullptr;
	Shader*					m_computeDensityCS = nullptr;
	Shader*					m_computeLambdaCS = nullptr;
	Shader*					m_computeDeltaPosCS = nullptr;
	Shader*					m_updateVelWithPosCS = nullptr;
	Shader*					m_applyViscosityCS = nullptr;
	Shader*					m_applyVorticityCS = nullptr;
	Shader*					m_bilateralFilterCS = nullptr;
	//Shader*					m_computeShader = nullptr;
	Shader*					m_renderShader = nullptr;
	Shader*					m_kuwaharaFilter = nullptr;
	Shader*					m_calculateCausticCS = nullptr;
	Shader*					m_generateCaustics = nullptr;

	Texture*				m_depthTexture = nullptr;
	Texture*				m_depthBlurredTexture = nullptr;
	Texture*				m_depthFinalTexture = nullptr;
	Texture*				m_thicknessTexture = nullptr;
	Texture*				m_thicknessBlurredTexture = nullptr;
	Texture*				m_normalTexture = nullptr;
	Texture*				m_backgroundTexture = nullptr;
	Texture*				m_finalTexture = nullptr;

	Texture*				m_sandDiffuse = nullptr;
	Texture*				m_sandNormal = nullptr;
	StructuredBuffer*		m_causticsProjectPosBuffer = nullptr;
	Texture*				m_causticsProjectPosTexture = nullptr;

	Texture*				m_lightDepthTexture = nullptr;
	Texture*				m_lightBlurredTexture = nullptr;
	Texture*				m_lightNormalTexture = nullptr;

	Texture*				m_skyCubeTexture = nullptr;

	VertexBuffer*			m_skyboxVBO;
	IndexBuffer*			m_skyboxIBO;
	Mat44					m_skyboxModelMat;

	Shader*					m_blurShader = nullptr;
	Shader*					m_calculateNormalShader = nullptr;
	Shader*					m_generateThickness = nullptr;
	Shader*					m_fluidMerge = nullptr;
	Shader*					m_skyboxShader = nullptr;

	StructuredBuffer*		m_particleBuffer = nullptr;
	const int				m_particleSlot = 1;

	StructuredBuffer*		m_spatialIndiceBuffer = nullptr;
	const int				m_spatialIndiceSlot = 2;

	StructuredBuffer*		m_spatialOffsetBuffer = nullptr;
	const int				m_spatialOffsetSlot = 3;

	StructuredBuffer*		m_densityBuffer = nullptr;
	const int				m_densitySlot = 4;

	StructuredBuffer*		m_lambdaBuffer = nullptr;
	const int				m_lambdaSlot = 5;

	Texture*				m_texture = nullptr;

	Camera					m_lightCamera;
};