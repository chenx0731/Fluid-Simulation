#include "Game/FluidSolver.hpp"
#include "Game/GameCommon.hpp"
#include "Game/NeighborSearch.hpp"
#include "Game/SPHKernels.hpp"
#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include "Game/PositionBasedFluid.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/StructuredBuffer.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/ResourceView.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Time.hpp"



struct Entry 
{
	int index;
	int hash;
	int key;
};

FluidSolver::FluidSolver()
{
	m_density0 = 1000.0f;
	m_maxIter = 3;
	m_viscosity = 0.3f;
	SetParticleRadius(0.05f);
	//m_particleRadius = 0.05;
	//m_sphRadius = 0.025;
	m_clock = new Clock(Clock::GetSystemClock());
	//m_spheres.reserve(20000);
	m_gpuMesh = nullptr;
	m_gpuMesh = g_theRenderer->CreateVertexBuffer(sizeof(Vertex_PCU), sizeof(Vertex_PCU));
	
	CreateComputeShaders();
	
	CreateShaders();

	CreateTextures();
	//CreateComputeShaders();
		//CreateShader("Data/Shaders/ScreenSpaceSphere");
	m_texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/Disc.png");
	m_neighborSearch = new NeighborSearch();

	std::vector<Vertex_PCUTBN> verts;
	std::vector<unsigned int> indexes;
	AddVertsForSkybox(verts, indexes, m_skyboxModelMat, 3.f, Vec3());

	m_skyboxVBO = g_theRenderer->CreateVertexBuffer(verts.size() * sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	m_skyboxIBO = g_theRenderer->CreateIndexBuffer(sizeof(unsigned int) * indexes.size());

	g_theRenderer->CopyCPUToGPU(verts.data(), verts.size() * sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN), m_skyboxVBO);
	g_theRenderer->CopyCPUToGPU(indexes.data(), indexes.size() * sizeof(unsigned int), m_skyboxIBO);

	
}

FluidSolver::~FluidSolver()
{
	ClearModel();
	delete m_clock;
	m_clock = nullptr;
	delete m_gpuMesh;
	m_gpuMesh = nullptr;
	
	delete m_particleBuffer;
	m_particleBuffer = nullptr;

	delete m_spatialIndiceBuffer;
	m_spatialIndiceBuffer = nullptr;

	delete m_spatialOffsetBuffer;
	m_spatialOffsetBuffer = nullptr;

	delete m_lambdaBuffer;
	m_lambdaBuffer = nullptr;

	delete m_densityBuffer;
	m_densityBuffer = nullptr;

	m_texture = nullptr;
	m_sandNormal = nullptr;
	m_sandDiffuse = nullptr;
	
	m_applyGravityCS = nullptr;
	m_spatialHashCS = nullptr;
	m_sortCS = nullptr;
	m_groupShareSortCS = nullptr;
	m_computeOffsetCS = nullptr;
	m_computeDensityCS = nullptr;
	m_computeLambdaCS = nullptr;
	m_computeDeltaPosCS = nullptr;
	m_updateVelWithPosCS = nullptr;
	m_applyViscosityCS = nullptr;
	m_renderShader = nullptr;
	m_kuwaharaFilter = nullptr;
	m_calculateCausticCS = nullptr;

	delete m_skyboxVBO;
	m_skyboxVBO = nullptr;
	delete m_skyboxIBO;
	m_skyboxIBO = nullptr;
	//DX_SAFE_RELEASE(m_shader)
}

void FluidSolver::SetUpModel()
{
	int num = 32;
	int fluidParticle = num * num * num  ;
	m_fluidNumOfParticles = num * num * num ;
		//num * num * num;
	int wallParticle = 0;
		//num * num * 5 * 4;
	m_numOfParticles = wallParticle + fluidParticle;
	m_particles.resize(m_numOfParticles);
	//for (int i = 0; i < fluidParticle + wallParticle; i++)
	//{
	//	m_particles[i] = new Particle();
	//}
	//m_particles.reserve(fluidParticle + wallParticle);
	for (int x = 0; x < num; x++) {
		for (int y = 0; y < num; y++) {
			for (int z = 0; z < num  ; z++) {
				int index = x + num * y + num*num * z;
				float x_float = (float)x * (float)m_rigidParticleRadius * 2.f + 1.f;
				float y_float = (float)y * (float)m_rigidParticleRadius * 2.f + 1.f;
				float z_float = (float)z * (float)m_rigidParticleRadius * 2.f + 2.f;
				m_particles[index].SetPosition(Vec3(x_float, y_float, z_float));
			}
		}
	}
	int index = fluidParticle;
	/*
	for (int x = 0; x < 2 * num; x++) {
		for (int y = 0; y < 2 * num; y++) {
				float x_float = (float)x * (float)m_rigidParticleRadius * 2.f;
				float y_float = (float)y * (float)m_rigidParticleRadius * 2.f;
				float z_float = -1.f * (float)m_rigidParticleRadius * 2.f;
				m_particles[index++].SetPosition(Vec3(x_float, y_float, z_float));
				m_particles[index++].SetPosition(Vec3(z_float, x_float, y_float));
				m_particles[index++].SetPosition(Vec3(x_float, z_float, y_float));
				z_float = (float)num * (float)m_rigidParticleRadius * 4.f;
				m_particles[index++].SetPosition(Vec3(z_float, x_float, y_float));
				m_particles[index++].SetPosition(Vec3(x_float, z_float, y_float));
		}
	}
*/
	// search solid particles' neighbors
	// handle boundary
	m_neighborSearch->SetRadius(m_sphRadius);
	m_neighborSearch->UpdateNeighbors(m_particles, fluidParticle, wallParticle);

	for (int i = fluidParticle; i < fluidParticle + wallParticle; i++) {

		const std::vector<unsigned int>& neighbors = m_neighborSearch->GetNeighbors(i);
		float sum = SPHKernel::Poly6(Vec3(), m_sphRadius);
		for (int neighborIndex = 0; neighborIndex < (int)neighbors.size(); neighborIndex++) {
			unsigned int neighbor = neighbors[neighborIndex];
			if (neighbor >= fluidParticle)
				sum += SPHKernel::Poly6(m_particles[i].GetPosition() - m_particles[neighbor].GetPosition(), m_sphRadius);
		}
		float mass = 1.0f / sum;
		m_particles[i].SetMass(m_density0 * mass);
	}
	m_neighborSearch->UpdateTimestamp();
	InitMasses(fluidParticle);
	m_lambda.resize(fluidParticle);
	m_density.resize(fluidParticle);
	
	m_particleBuffer = g_theRenderer->CreateStructuredBufferByData(m_particles.size(), sizeof(Particle), m_particles.data());
	m_spatialIndiceBuffer = g_theRenderer->CreateStructuredBufferByData(m_particles.size(), sizeof(Entry), nullptr);
	m_spatialOffsetBuffer = g_theRenderer->CreateStructuredBufferByData(m_particles.size(), sizeof(unsigned int), nullptr);
	m_densityBuffer = g_theRenderer->CreateStructuredBufferByData(fluidParticle, sizeof(float), nullptr);
	m_lambdaBuffer = g_theRenderer->CreateStructuredBufferByData(fluidParticle, sizeof(float), nullptr);
	m_vertexCount = (int)m_particles.size();

	m_lightCamera = g_theGame->m_player->m_camera;
	m_lightCamera.m_position = Vec3(5.f, 13.f, 20.f);
	m_lightCamera.m_orientation = EulerAngles(0.f, 90.f, 0.f);
}

void FluidSolver::ClearModel()
{
	//for (int i = 0; i < (int)m_particles.size(); i++) {
	//	if (m_particles[i] != nullptr) {
	//		delete m_particles[i];
	//		m_particles[i] = nullptr;
	//	}
	//}
	std::vector<Particle>().swap(m_particles);
	std::vector<float>().swap(m_lambda);
	std::vector<float>().swap(m_density);
	std::vector<Vec3>().swap(m_deltaPos);
	delete m_particleBuffer;
	m_particleBuffer = nullptr;
	delete m_spatialIndiceBuffer;
	m_spatialIndiceBuffer = nullptr;
	delete m_spatialOffsetBuffer;
	m_spatialOffsetBuffer = nullptr;
	delete m_densityBuffer;
	m_densityBuffer = nullptr;
	delete m_lambdaBuffer;
	m_lambdaBuffer = nullptr;
}

void FluidSolver::ResetSimulation()
{
	ClearModel();
	SetUpModel();

}

void FluidSolver::ResetParticles(const int& size)
{
	m_particles.resize(size);
	m_lambda.resize(size);
	m_density.resize(size);
	m_deltaPos.resize(size);
}

void FluidSolver::SetParticleRadius(const float& radius)
{
	m_rigidParticleRadius = radius;
	m_sphRadius = radius * 4.0f - 4e-3f;
	m_particleRadius = 0.125f;
	//m_density0 = 1.f / (8.f * 0.05f * 0.05f * 0.05f);
}

void FluidSolver::CreateComputeShaders()
{
	ShaderConfig config;
	config.m_name = "Data/Shaders/PBFSim";
	config.m_hasCS = true;
	config.m_hasVS = false;
	config.m_hasPS = false;
	config.m_computeEntryPoint = "UpdateSpatialHash";
	m_spatialHashCS		= g_theRenderer->CreateShaderByConfig(config);
	config.m_computeEntryPoint = "ApplyGravity";
	m_applyGravityCS	= g_theRenderer->CreateShaderByConfig(config);
	config.m_computeEntryPoint = "ComputeDensity";
	m_computeDensityCS	= g_theRenderer->CreateShaderByConfig(config);
	config.m_computeEntryPoint = "ComputeLambda";
	m_computeLambdaCS	= g_theRenderer->CreateShaderByConfig(config);
	config.m_computeEntryPoint = "ComputeDeltaPos";
	m_computeDeltaPosCS = g_theRenderer->CreateShaderByConfig(config);
	config.m_computeEntryPoint = "UpdateVelocityByPosition";
	m_updateVelWithPosCS = g_theRenderer->CreateShaderByConfig(config);
	//config.m_computeEntryPoint = "ApplyVorticityConfinement";
	//m_applyVorticityCS	= g_theRenderer->CreateShaderByConfig(config);
	config.m_computeEntryPoint = "ApplyViscosity";
	m_applyViscosityCS	= g_theRenderer->CreateShaderByConfig(config);

	config.m_name = "Data/Shaders/BitonicMergeSort";
	config.m_computeEntryPoint = "Sort";
	m_sortCS = g_theRenderer->CreateShaderByConfig(config);
	config.m_computeEntryPoint = "CalculateOffsets";
	m_computeOffsetCS = g_theRenderer->CreateShaderByConfig(config);

	config.m_name = "Data/Shaders/SortCS";
	config.m_computeEntryPoint = "BitonicSortLDS";
	m_groupShareSortCS = g_theRenderer->CreateShaderByConfig(config);

	config.m_name = "Data/Shaders/BilateralFilterCS";
	config.m_computeEntryPoint = "BilateralCompletionCS";
	m_bilateralFilterCS = g_theRenderer->CreateShaderByConfig(config);
		//CreateComputeShader("Data/Shaders/CSApplyGravity.hlsl");
	config.m_name = "Data/Shaders/CalculateCausticCS";
	config.m_computeEntryPoint = "CalculateCausticIntersection";
	m_calculateCausticCS = g_theRenderer->CreateShaderByConfig(config);
}

void FluidSolver::CreateShaders()
{
	ShaderConfig config;
	config.m_name = "Data/Shaders/BlurDepthTexture";
	config.m_hasCS = false;
	config.m_hasVS = true;
	config.m_hasPS = true;
	config.m_vertexEntryPoint = "VertexMain";
	config.m_pixelEntryPoint = "PixelMain";
	m_blurShader = g_theRenderer->CreateShaderByConfig(config);

	config.m_name = "Data/Shaders/ScreenSpaceSphere";
	config.m_hasGS = true;
	m_renderShader = g_theRenderer->CreateShaderByConfig(config);


	config.m_name = "Data/Shaders/CalculateThickness";
	//config.m_hasGS = true;
	m_generateThickness = g_theRenderer->CreateShaderByConfig(config);

	config.m_name = "Data/Shaders/GenerateCaustics";
	m_generateCaustics = g_theRenderer->CreateShaderByConfig(config);

	config.m_name = "Data/Shaders/CalculateNormalMap";
	config.m_hasGS = false;
	m_calculateNormalShader = g_theRenderer->CreateShaderByConfig(config);

	config.m_name = "Data/Shaders/FluidFinalMerge1";
	m_fluidMerge = g_theRenderer->CreateShaderByConfig(config);

	config.m_name = "Data/Shaders/Kuwahara";
	m_kuwaharaFilter = g_theRenderer->CreateShaderByConfig(config);

	config.m_name = "Data/Shaders/Skybox";
	m_skyboxShader = g_theRenderer->CreateShaderByConfig(config);

}

void FluidSolver::CreateTextures()
{
	TextureConfig depth;
	depth.m_name = "Fluid Depth Texture";
	depth.m_width = g_theWindow->GetClientDimensions().x;
	depth.m_height = g_theWindow->GetClientDimensions().y;
	depth.m_format = ResourceFormat::R24G8_TYPELESS;
	depth.m_bindFlags = RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW | RESOURCE_BINDFLAGS_DEPTH_STENCIL_VIEW;
	depth.m_usage = MemoryUsage::DEFAULT;

	m_depthTexture = g_theRenderer->CreateTextureByConfig(depth);
	m_depthTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_DEPTH_STENCIL_VIEW);
	m_depthTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW);

	m_lightDepthTexture = g_theRenderer->CreateTextureByConfig(depth);
	m_lightDepthTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_DEPTH_STENCIL_VIEW);
	m_lightDepthTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW);

	TextureConfig blurred;
	blurred.m_name = "Fluid Blurred Texture";
	blurred.m_width = g_theWindow->GetClientDimensions().x;
	blurred.m_height = g_theWindow->GetClientDimensions().y;
	blurred.m_format = ResourceFormat::R32_FLOAT;
	blurred.m_bindFlags = RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW | RESOURCE_BINDFLAGS_UNORDERED_ACCESS_VIEW;
	blurred.m_usage = MemoryUsage::DEFAULT;

	m_depthBlurredTexture = g_theRenderer->CreateTextureByConfig(blurred);
	m_depthBlurredTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW);
	m_depthBlurredTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_UNORDERED_ACCESS_VIEW);

	m_lightBlurredTexture = g_theRenderer->CreateTextureByConfig(blurred);
	m_lightBlurredTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW);
	m_lightBlurredTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_UNORDERED_ACCESS_VIEW);

	blurred.m_name = "Fluid Final Blurred Texture";
	blurred.m_bindFlags = RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW | RESOURCE_BINDFLAGS_UNORDERED_ACCESS_VIEW;
	m_depthFinalTexture = g_theRenderer->CreateTextureByConfig(blurred);
	m_depthFinalTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW);
	m_depthFinalTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_UNORDERED_ACCESS_VIEW);

	TextureConfig thickness;
	thickness.m_name = "Fluid Thickness Texture";
	thickness.m_width = g_theWindow->GetClientDimensions().x;
	thickness.m_height = g_theWindow->GetClientDimensions().y;
	thickness.m_format = ResourceFormat::R8G8B8A8_UNORM;
	thickness.m_bindFlags = RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW | RESOURCE_BINDFLAGS_RENDER_TARGET_VIEW;
	thickness.m_usage = MemoryUsage::DEFAULT;
	
	m_thicknessTexture = g_theRenderer->CreateTextureByConfig(thickness);
	m_thicknessTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW);
	m_thicknessTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_RENDER_TARGET_VIEW);

	thickness.m_name = "Fluid Thickness Blurred Texture";
	thickness.m_bindFlags = RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW | RESOURCE_BINDFLAGS_RENDER_TARGET_VIEW;
	m_thicknessBlurredTexture = g_theRenderer->CreateTextureByConfig(thickness);
	m_thicknessBlurredTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW);
	m_thicknessBlurredTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_RENDER_TARGET_VIEW);

	TextureConfig normal;
	normal.m_name = "Fluid Normal Texture";
	normal.m_width = g_theWindow->GetClientDimensions().x;
	normal.m_height = g_theWindow->GetClientDimensions().y;
	normal.m_format = ResourceFormat::R8G8B8A8_UNORM;
	normal.m_bindFlags = RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW | RESOURCE_BINDFLAGS_RENDER_TARGET_VIEW;
	normal.m_usage = MemoryUsage::DEFAULT;

	m_normalTexture = g_theRenderer->CreateTextureByConfig(normal);

	m_normalTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW);
	m_normalTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_RENDER_TARGET_VIEW);

	m_lightNormalTexture = g_theRenderer->CreateTextureByConfig(normal);

	m_lightNormalTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW);
	m_lightNormalTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_RENDER_TARGET_VIEW);

	TextureConfig impactPos;
	impactPos.m_name = "Fluid Normal Texture";
	impactPos.m_width = g_theWindow->GetClientDimensions().x;
	impactPos.m_height = g_theWindow->GetClientDimensions().y;
	impactPos.m_format = ResourceFormat::R32G32_FlOAT;
	impactPos.m_bindFlags = RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW | RESOURCE_BINDFLAGS_UNORDERED_ACCESS_VIEW;
	impactPos.m_usage = MemoryUsage::DEFAULT;

	m_causticsProjectPosTexture = g_theRenderer->CreateTextureByConfig(impactPos);
	m_causticsProjectPosTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW);
	m_causticsProjectPosTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_UNORDERED_ACCESS_VIEW);

	TextureConfig background;
	Image bg = Image("Data/Images/nature.jpg");
	background.m_name = "Background Texture";
	background.m_width = g_theWindow->GetClientDimensions().x;
	background.m_height = g_theWindow->GetClientDimensions().y;
	background.m_format = ResourceFormat::R8G8B8A8_UNORM;
	background.m_initialData = (void*)bg.GetRawData();
	background.m_stride = bg.GetDimensions().x * 4;
	background.m_bindFlags = RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW | RESOURCE_BINDFLAGS_RENDER_TARGET_VIEW;
	background.m_usage = MemoryUsage::DEFAULT;

	m_backgroundTexture = g_theRenderer->CreateTextureByConfig(background);
	m_backgroundTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_RENDER_TARGET_VIEW);
	m_backgroundTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW);

	Image sandDiffuse = Image("Data/Images/sandDiffuse.jpg");
	m_sandDiffuse = g_theRenderer->CreateTextureFromImage(sandDiffuse);
	Image sandNormal = Image("Data/Images/sandNormal.jpg");
	m_sandNormal = g_theRenderer->CreateTextureFromImage(sandNormal);

	TextureConfig final;
	final.m_name = "Final Texture";
	final.m_width = g_theWindow->GetClientDimensions().x;
	final.m_height = g_theWindow->GetClientDimensions().y;
	final.m_format = ResourceFormat::R8G8B8A8_UNORM;
	final.m_bindFlags = RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW | RESOURCE_BINDFLAGS_RENDER_TARGET_VIEW;
	final.m_usage = MemoryUsage::DEFAULT;

	m_finalTexture = g_theRenderer->CreateTextureByConfig(final);

	m_finalTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_SHADER_RESOURCE_VIEW);
	m_finalTexture->GetOrCreateResourceViewByFlag(RESOURCE_BINDFLAGS_RENDER_TARGET_VIEW);

	Strings skycube;
	
	skycube.push_back(Stringf("Data/Images/Skybox/0Left.bmp"));
	skycube.push_back(Stringf("Data/Images/Skybox/1Right.bmp"));
	skycube.push_back(Stringf("Data/Images/Skybox/2Front.bmp"));
	skycube.push_back(Stringf("Data/Images/Skybox/3Back.bmp"));
	skycube.push_back(Stringf("Data/Images/Skybox/4Top.bmp"));
	skycube.push_back(Stringf("Data/Images/Skybox/5Bottom.bmp"));

	//skycube.push_back(Stringf("Data/Images/Skybox/Lycksele/posx.jpg"));
	//skycube.push_back(Stringf("Data/Images/Skybox/Lycksele/negx.jpg"));
	//skycube.push_back(Stringf("Data/Images/Skybox/Lycksele/posz.jpg"));
	//skycube.push_back(Stringf("Data/Images/Skybox/Lycksele/negz.jpg"));
	//skycube.push_back(Stringf("Data/Images/Skybox/Lycksele/posy.jpg"));
	//skycube.push_back(Stringf("Data/Images/Skybox/Lycksele/negy.jpg"));
	m_skyCubeTexture = g_theRenderer->CreateCubemapTextureFromFiles(skycube);
}

void FluidSolver::InitMasses(const int& size)
{
	float volume = 8.0f * m_particleRadius * m_particleRadius * m_particleRadius; // 2^3 * r^3
	float mass = volume * m_density0;
	mass = 1.f;
	for (int i = 0; i < size; i++) {
		m_particles[i].SetMass(mass);
	}
}

void FluidSolver::AddGravity()
{
	const Vec3 gravity = Vec3(0.f, 0.f, -9.8f);
	for (int i = 0; i < (int)m_particles.size(); i++) {
		if (m_particles[i].GetMass() != 0.0) {
			m_particles[i].SetAcceleration(gravity);
		}
	}
}

void FluidSolver::SortHashMap()
{
	/*
	g_theRenderer->CSSetUAV(m_spatialIndiceBuffer, m_spatialIndiceSlot);
	g_theRenderer->CSSetUAV(m_spatialOffsetBuffer, m_spatialOffsetSlot);
	g_theRenderer->CSSetShader(m_groupShareSortCS);
	SortConstants sortConstants;
	sortConstants.m_groupWidth = 0;
	sortConstants.m_groupHeight = 0;
	sortConstants.m_stepIndex = 0;
	sortConstants.m_numOfEntries = (int)m_particles.size();

	g_theRenderer->CopyCPUToGPU(&sortConstants, sizeof(SortConstants), g_theGame->m_sortCBO);
	g_theRenderer->CSBindConstantBuffer(5, g_theGame->m_sortCBO);

	g_theRenderer->DispatchCS(m_numOfParticles / (256), 1, 1);*/
	int numStages = (int)Log2(NextPowerOfTwo((float)m_particles.size()));
	//numStages += 1;
	g_theRenderer->CSSetUAV(m_spatialIndiceBuffer, m_spatialIndiceSlot);
	g_theRenderer->CSSetUAV(m_spatialOffsetBuffer, m_spatialOffsetSlot);
	g_theRenderer->CSSetShader(m_sortCS);
	for (int stageIndex = 0; stageIndex < numStages; stageIndex++) {
		for (int stepIndex = 0; stepIndex < stageIndex + 1; stepIndex++) {
			int groupWidth = 1 << (stageIndex - stepIndex);
			int groupHeight = 2 * groupWidth - 1;

			//g_theRenderer->CSSetShader(m_sortCS);

			SortConstants sortConstants;
			sortConstants.m_groupWidth = groupWidth;
			sortConstants.m_groupHeight = groupHeight;
			sortConstants.m_stepIndex = stepIndex;
			sortConstants.m_numOfEntries = (int)m_particles.size();

			g_theRenderer->CopyCPUToGPU(&sortConstants, sizeof(SortConstants), g_theGame->m_sortCBO);

			g_theRenderer->CSBindConstantBuffer(5, g_theGame->m_sortCBO);

			//g_theRenderer->CSSetUAV(m_spatialIndiceBuffer, 2);

			// #ChangeNumOfThd : change value of X later
			g_theRenderer->DispatchCS(m_numOfParticles / (m_numOfThreads * 2 * 2), 1, 1);

			//g_theRenderer->CSSetUAV(nullptr, 2);
		}
	}

	// Calculate offset 
	g_theRenderer->CSSetShader(m_computeOffsetCS);
	// #ChangeNumOfThd : change value of X later
	g_theRenderer->DispatchCS(m_numOfParticles / (m_numOfThreads * 2), 1, 1);
	//g_theRenderer->CSSetUAV(nullptr, m_);
	//g_theRenderer->CSSetUAV(nullptr, 3);

}

void FluidSolver::ProjectConstraint()
{
	unsigned int iter = 0;
	int fluidParticleNum = (int)m_lambda.size();
	if (m_isApplyingPressure) {
		while (iter < m_maxIter) {
			for (int i = 0; i < fluidParticleNum; i++) {
				PositionBasedFluid::CalculateDensity(i, fluidParticleNum, m_sphRadius, m_particles, m_neighborSearch->GetNeighbors(i), m_density0, m_density[i]);
			}

			for (int i = 0; i < fluidParticleNum; i++) {
				PositionBasedFluid::CalculateLagrangeMultiplier(i, fluidParticleNum, m_sphRadius, m_particles, m_neighborSearch->GetNeighbors(i), m_density0, m_density[i], m_lambda[i]);
			}

			for (int i = 0; i < fluidParticleNum; i++) {
				Vec3 newPos = m_particles[i].GetPosition();
				PositionBasedFluid::CalculateDeltaPos(i, fluidParticleNum, m_sphRadius, m_particles, m_neighborSearch->GetNeighbors(i), m_density0, m_lambda, newPos);
				m_particles[i].SetPosition(newPos);
			}
			iter++;
		}
	}

}

void FluidSolver::ApplyViscosity(int particleIndex)
{
	//for (int i = 0; i < (int)m_density.size(); i++) {
	const std::vector<unsigned int>& neighbors = m_neighborSearch->GetNeighbors(particleIndex);
	Vec3 deltaVel = Vec3();
	for (int j = 0; j < (int)neighbors.size(); j++) {
		unsigned int neighborIndex = neighbors[j];
		if (neighborIndex >= 1000)
			continue;
		// vj - vi
		Vec3 vel_IJ = m_particles[neighborIndex].GetVelocity() - m_particles[particleIndex].GetVelocity();
		Vec3 r = m_particles[particleIndex].GetPosition() - m_particles[neighborIndex].GetPosition();
		double kernel = SPHKernel::Poly6(r, m_sphRadius);
		deltaVel += vel_IJ * static_cast<float>(m_viscosity * kernel * (m_particles[neighborIndex].GetMass() / m_density0));
	}
	Vec3 nowVel = m_particles[particleIndex].GetVelocity();
	m_particles[particleIndex].SetVelocity(nowVel + deltaVel);
	//}
}

void FluidSolver::Update()
{
	if (m_isUsingGPU) {
		SetSimulatorCBO();
		
		// apply gravity
		g_theRenderer->CSSetUAV(m_particleBuffer, m_particleSlot);
		g_theRenderer->CSSetShader(m_applyGravityCS);
		g_theRenderer->DispatchCS(m_fluidNumOfParticles / m_numOfThreads, 1, 1);
		g_theRenderer->CSSetUAV((StructuredBuffer*)nullptr, m_particleSlot);
		

		// spatial hash
		g_theRenderer->CSSetSRV(m_particleBuffer, m_particleSlot);
		g_theRenderer->CSSetUAV(m_spatialIndiceBuffer, m_spatialIndiceSlot);
		g_theRenderer->CSSetUAV(m_spatialOffsetBuffer, m_spatialOffsetSlot);
		g_theRenderer->CSSetShader(m_spatialHashCS);
		//g_theRenderer->CSSetUAV(m_particleBuffer, 1);
		g_theRenderer->DispatchCS(m_numOfParticles / m_numOfThreads + 1, 1, 1);
		g_theRenderer->CSSetSRV((StructuredBuffer*)nullptr, m_particleSlot);
		g_theRenderer->CSSetUAV((StructuredBuffer*)nullptr, m_spatialIndiceSlot);
		g_theRenderer->CSSetUAV((StructuredBuffer*)nullptr, m_spatialOffsetSlot);
		//g_theRenderer->CSSetUAV(nullptr, 1);
		//g_theRenderer->CSSetSRV(nullptr, m_particleSlot);


		// sort
		if (m_isApplyingPressure) {
			double start = GetCurrentTimeSeconds();
			SortHashMap();
			double totalSort = GetCurrentTimeSeconds() - start;
			//DebuggerPrintf(Stringf("sort: %.2fms\n", (totalSort * 1000)).c_str());
			unsigned int iter = 0;
			double startCal = GetCurrentTimeSeconds();
			while (iter < m_maxIter) {
				// compute density
				g_theRenderer->CSSetSRV(m_particleBuffer, m_particleSlot);
				g_theRenderer->CSSetUAV(m_densityBuffer, m_densitySlot);
				g_theRenderer->CSSetShader(m_computeDensityCS);
				g_theRenderer->DispatchCS(m_fluidNumOfParticles / m_numOfThreads, 1, 1);
				g_theRenderer->CSSetUAV((StructuredBuffer*)nullptr, m_densitySlot);
				// compute lambda
				g_theRenderer->CSSetSRV(m_densityBuffer, m_densitySlot);
				g_theRenderer->CSSetUAV(m_lambdaBuffer, m_lambdaSlot);
				g_theRenderer->CSSetShader(m_computeLambdaCS);
				g_theRenderer->DispatchCS(m_fluidNumOfParticles / m_numOfThreads, 1, 1);
				g_theRenderer->CSSetUAV((StructuredBuffer*)nullptr, m_lambdaSlot);
				g_theRenderer->CSSetSRV((StructuredBuffer*)nullptr, m_densitySlot);
				g_theRenderer->CSSetSRV((StructuredBuffer*)nullptr, m_particleSlot);
				// compute delta pos

				g_theRenderer->CSSetSRV(m_lambdaBuffer, m_lambdaSlot);
				g_theRenderer->CSSetUAV(m_particleBuffer, m_particleSlot);
				g_theRenderer->CSSetShader(m_computeDeltaPosCS);
				g_theRenderer->DispatchCS(m_fluidNumOfParticles / m_numOfThreads, 1, 1);
				g_theRenderer->CSSetSRV((StructuredBuffer*)nullptr, m_lambdaSlot);
				g_theRenderer->CSSetUAV((StructuredBuffer*)nullptr, m_particleSlot);
				iter++;
			}
			double totalCal = GetCurrentTimeSeconds() - startCal;
			//DebuggerPrintf(Stringf("cal: %.2fms\n", (totalCal * 1000)).c_str());
		}
		

		// Update Velocity
		g_theRenderer->CSSetUAV(m_particleBuffer, m_particleSlot);
		g_theRenderer->CSSetShader(m_updateVelWithPosCS);
		g_theRenderer->DispatchCS(m_fluidNumOfParticles / m_numOfThreads, 1, 1);

		// Apply Viscosity
		if (m_isApplyingViscosity) {
			g_theRenderer->CSSetShader(m_applyViscosityCS);
			g_theRenderer->DispatchCS(m_fluidNumOfParticles / m_numOfThreads, 1, 1);
		}

		// Apply vorticity
		if (m_isApplyingVorticity) {

		}

		g_theRenderer->CSSetUAV((StructuredBuffer*)nullptr, m_particleSlot);
		g_theRenderer->CSSetUAV((StructuredBuffer*)nullptr, m_spatialIndiceSlot);
		g_theRenderer->CSSetUAV((StructuredBuffer*)nullptr, m_spatialOffsetSlot);
		g_theRenderer->CSSetUAV((StructuredBuffer*)nullptr, m_densitySlot);
		g_theRenderer->CSSetUAV((StructuredBuffer*)nullptr, m_lambdaSlot);

		g_theRenderer->CSSetSRV((StructuredBuffer*)nullptr, m_particleSlot);
		g_theRenderer->CSSetSRV((StructuredBuffer*)nullptr, m_spatialIndiceSlot);
		g_theRenderer->CSSetSRV((StructuredBuffer*)nullptr, m_spatialOffsetSlot);
		g_theRenderer->CSSetSRV((StructuredBuffer*)nullptr, m_densitySlot);
		g_theRenderer->CSSetSRV((StructuredBuffer*)nullptr, m_lambdaSlot);
		//SetRenderCBO();
	}
	else {
		// Add gravity
		AddGravity();

		// Update Velocity and Position(Semi-implicit Euler)
		for (int i = 0; i < (int)m_density.size(); i++) {
			//(int)m_particles.size(); i++) {
			if (m_particles[i].GetMass() != 0.0) {
				float deltaTime = m_clock->GetDeltaSeconds();
				if (deltaTime > m_timeStep)
					deltaTime = m_timeStep;
				m_particles[i].SetLastPosition(m_particles[i].GetOldPosition());
				m_particles[i].SetOldPosition(m_particles[i].GetPosition());
				Vec3 vel = m_particles[i].GetVelocity();
				m_particles[i].SetVelocity(m_particles[i].GetAcceleration() * deltaTime + vel);
				m_particles[i].SetPosition(m_particles[i].GetPosition() + m_particles[i].GetVelocity() * deltaTime);

			}
		}

		// Neighbor Searching
		m_neighborSearch->SetRadius(m_sphRadius);
		m_neighborSearch->UpdateNeighbors(m_particles, 0, 1000);
		// Perform density constraint
		ProjectConstraint();
		// update velocity using position - oldPosition
		for (int i = 0; i < (int)m_lambda.size(); i++) {
			float deltaSecond = m_clock->GetDeltaSeconds();
			if (m_timeStep < deltaSecond)
				deltaSecond = m_timeStep;
			if (deltaSecond == 0.f)
				deltaSecond = 0.01f;
			m_particles[i].UpdateVelocityByPosition(deltaSecond);
			//if (m_isApplyingViscosity) {
			//	ApplyViscosity(i);
			//}
		}
		// apply vorticity confinement and XSPH viscosity
		for (int i = 0; i < (int)m_lambda.size(); i++) {
			if (m_isApplyingViscosity) {
				ApplyViscosity(i);
			}
		}

		// update time step
		m_neighborSearch->UpdateTimestamp();

		// Update Mesh
	}

	std::vector<Vertex_PCU> spheres, walls;
	float particleRadius = static_cast<float>(m_particleRadius);
	AddVertsForAABB3D(walls, AABB3(Vec3(0.f, 0.f, -particleRadius), Vec3(particleRadius * 40.f, particleRadius * 40.f, particleRadius)), Rgba8(0, 0, 255, 127));
	AddVertsForAABB3D(walls, AABB3(Vec3(0.f, 0.f, -particleRadius), Vec3(particleRadius * 40.f, particleRadius, particleRadius * 40.f)), Rgba8(0, 0, 255, 127));
	AddVertsForAABB3D(walls, AABB3(Vec3(0.f, 0.f, -particleRadius), Vec3(particleRadius, particleRadius * 40.f, particleRadius * 40.f)), Rgba8(0, 0, 255, 127));
	AddVertsForAABB3D(walls, AABB3(Vec3(-particleRadius + particleRadius * 40.f, 0.f, 0.f), Vec3(particleRadius + particleRadius * 40.f, particleRadius * 40.f, particleRadius * 40.f)), Rgba8(0, 0, 255, 127));
	AddVertsForAABB3D(walls, AABB3(Vec3(0.f, -particleRadius + particleRadius * 40.f, 0.f), Vec3(particleRadius * 40.f, particleRadius + particleRadius * 40.f, particleRadius * 40.f)), Rgba8(0, 0, 255, 127));
	//spheres.insert(spheres.end(), walls.begin(), walls.end());

	if (!m_isUsingGPU) {

		std::vector<Vertex_PCU> fluidSphere, wallSphere;
		AddVertsForSphere3D(fluidSphere, Vec3(), (float)m_particleRadius, Rgba8(200, 200, 255, 220), AABB2(), 6);
		AddVertsForSphere3D(wallSphere, Vec3(), (float)m_particleRadius, Rgba8(0, 0, 255, 127), AABB2(), 2);
		spheres.reserve(500000);
		for (int i = 0; i < (int)m_particles.size(); i++) {
			std::vector<Vertex_PCU> tempSphere;
			if (i < (int)m_density.size()) {
				tempSphere = fluidSphere;
				TransformVertexArrayXYZ3D((int)tempSphere.size(), tempSphere.data(), Vec3(1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, 1.f), m_particles[i].GetPosition());
				spheres.insert(spheres.end(), tempSphere.begin(), tempSphere.end());
				//spheres.insert(spheres.end(), tempSphere.begin(), tempSphere.end());
				//TransformPositionXYZ3D(tempSphere, Vec3(1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, 1.f), )
			}
			//AddVertsForSphere3D(spheres, m_particles[i].GetPosition(), (float)m_particleRadius);
			else {
				break;
				tempSphere = wallSphere;
			}
			//TransformVertexArrayXYZ3D((int)tempSphere.size(), tempSphere.data(), Vec3(1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, 1.f), m_particles[i]->GetPosition());
			spheres.insert(spheres.end(), tempSphere.begin(), tempSphere.end());
		}
		
		//walls = spheres;
	}
	spheres.insert(spheres.end(), walls.begin(), walls.end());
	g_theRenderer->CopyCPUToGPU(spheres.data(), spheres.size() * sizeof(Vertex_PCU), sizeof(Vertex_PCU), m_gpuMesh);
	m_vertexCount = (int)spheres.size();
	

}

void FluidSolver::Render() const
{
	SetFluidLightCBO();
	SetRenderCBO();
	//g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_BACK);
	RenderSkybox();
	//RenderSandPlane();
	RenderSurfaceInfoFromLight();

	if (m_isUsingGPU) {
		// Get depth texture
		g_theRenderer->UnbindDepthStencil();
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);
		g_theRenderer->ClearScreen(Rgba8(0, 0, 0, 0), m_depthTexture);
		g_theRenderer->SetDepthTexture(m_depthTexture);
		g_theRenderer->VSSetShader(m_renderShader);
		g_theRenderer->GSSetShader(m_renderShader);
		g_theRenderer->PSSetShader(m_renderShader);

		//g_theRenderer->VSBindConstantBuffer(k_renderConstantsSlot, m_renderCBO);
		//g_theRenderer->VSBindConstantBuffer(k_simConstantsSlot, m_simCBO);
		//g_theRenderer->GSBindConstantBuffer(k_renderConstantsSlot, m_renderCBO);
		//g_theRenderer->PSBindConstantBuffer(k_renderConstantsSlot, m_renderCBO);

		g_theRenderer->VSSetSRV(m_particleBuffer, m_particleSlot);
		g_theRenderer->VSSetSRV(m_densityBuffer, m_densitySlot);

		g_theRenderer->IASetInputLayout(m_renderShader);
		g_theRenderer->IASetVertexBuffers(nullptr);
		//g_theRenderer->IASetPrimitiveTopologyToPointList();
		g_theRenderer->IASetPrimitiveTopology(Topology::PointList);
		g_theRenderer->BindTexture(m_texture);

		g_theRenderer->Draw(m_fluidNumOfParticles, 0);
		// Get thickness texture
		g_theRenderer->ClearRenderTarget(Rgba8(0, 0, 0, 0), m_thicknessTexture);
		g_theRenderer->SetDepthTexture(nullptr, m_thicknessTexture);
		g_theRenderer->VSSetShader(m_generateThickness);
		g_theRenderer->GSSetShader(m_generateThickness);
		g_theRenderer->PSSetShader(m_generateThickness);
		g_theRenderer->BindTexture(nullptr);

		g_theRenderer->SetBlendMode(BlendMode::ADDITIVE);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->Draw(m_fluidNumOfParticles, 0);

		g_theRenderer->UnbindDepthStencil();

		// blur depth texture horizontally
		ApplyBilateralFilter();
		/*
		g_theRenderer->ClearRenderTarget(Rgba8(0, 0, 0, 0), m_depthBlurredTexture);
		g_theRenderer->SetDepthTexture(nullptr, m_depthBlurredTexture);
		g_theRenderer->BindShader(m_blurShader);
		g_theRenderer->BindTexture(m_depthTexture, 2);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->IASetPrimitiveTopology(Topology::TriangleList);
		//g_theRenderer->SetDepthMode(DepthMode::ENABLED);
		//SetBlurCBO(Vec2(0.0002f, 0.001f));
		SetBlurCBO(Vec2(1.f / (float)m_depthTexture->GetDimensions().x, 1.f / (float)m_depthTexture->GetDimensions().y));
		g_theRenderer->Draw(3, 0);

		g_theRenderer->ClearRenderTarget(Rgba8(0, 0, 0, 0), m_depthFinalTexture);
		g_theRenderer->SetDepthTexture(nullptr, m_depthFinalTexture);
		g_theRenderer->BindTexture(m_depthBlurredTexture, 2);
		//SetBlurCBO(Vec2(0.f, 1.f / (float)m_depthTexture->GetDimensions().y));
		g_theRenderer->Draw(3, 0);
		g_theRenderer->BindTexture(nullptr, 2);
	
		g_theRenderer->ClearRenderTarget(Rgba8(0, 0, 0, 0), m_depthBlurredTexture);
		g_theRenderer->SetDepthTexture(nullptr, m_depthBlurredTexture);
		g_theRenderer->BindTexture(m_depthFinalTexture, 2);
		//SetBlurCBO(Vec2(1.f / (float)m_depthTexture->GetDimensions().x, 1.f / (float)m_depthTexture->GetDimensions().y));
		g_theRenderer->Draw(3, 0);
		g_theRenderer->BindTexture(nullptr, 2);

		g_theRenderer->ClearRenderTarget(Rgba8(0, 0, 0, 0), m_depthFinalTexture);
		g_theRenderer->SetDepthTexture(nullptr, m_depthFinalTexture);
		//g_theRenderer->BindShader(nullptr);
		//g_theRenderer->BindShader(m_kuwaharaFilter);
		g_theRenderer->BindTexture(m_depthBlurredTexture, 2);
		//SetBlurCBO(Vec2(-1.f / (float)m_depthTexture->GetDimensions().x, 1.f / (float)m_depthTexture->GetDimensions().y));
		g_theRenderer->Draw(3, 0);
		g_theRenderer->BindTexture(nullptr, 2);

		*/
		
		// blur thickness texture
		
		g_theRenderer->ClearRenderTarget(Rgba8(0, 0, 0, 0), m_thicknessBlurredTexture);
		//SetBlurCBO(Vec2(0.f, 0.f));
		g_theRenderer->BindShader(m_blurShader);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->IASetPrimitiveTopology(Topology::TriangleList);
		//g_theRenderer->SetDepthMode(DepthMode::ENABLED);
		//SetBlurCBO(Vec2(0.0002f, 0.001f));
		SetBlurCBO(Vec2(1.f / (float)m_depthTexture->GetDimensions().x, 1.f / (float)m_depthTexture->GetDimensions().y));
		g_theRenderer->SetDepthTexture(nullptr, m_thicknessBlurredTexture);

		g_theRenderer->BindTexture(m_thicknessTexture, 2);
		g_theRenderer->Draw(3, 0);

		// blur depth texture vertically
		/*
		g_theRenderer->ClearRenderTarget(Rgba8(0, 0, 0, 0), m_depthFinalTexture);
		g_theRenderer->SetDepthTexture(nullptr, m_depthFinalTexture);
		g_theRenderer->BindTexture(m_depthBlurredTexture, 2);
		SetBlurCBO(Vec2(0.f, 1.f / (float)m_depthTexture->GetDimensions().y));
		g_theRenderer->Draw(3, 0);
		g_theRenderer->BindTexture(nullptr, 2);
		g_theRenderer->BindShader(nullptr);
		*/
		// Get Normal texture
		g_theRenderer->IASetPrimitiveTopology(Topology::TriangleList);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);
		g_theRenderer->ClearRenderTarget(Rgba8(0, 0, 0, 0), m_normalTexture);
		g_theRenderer->SetDepthTexture(nullptr, m_normalTexture);
		g_theRenderer->BindShader(m_calculateNormalShader);
		//g_theRenderer->BindTexture(m_depthFinalTexture, 2);
		g_theRenderer->BindTexture(m_depthFinalTexture, 2);
		
		//std::vector<Vertex_PCU> billboard;
		
		//DrawSquare(Vec2(-SCREEN_CAMERA_SIZE_X / 2.f, -SCREEN_CAMERA_SIZE_Y / 2.f), Vec2(SCREEN_CAMERA_SIZE_X / 2.f, SCREEN_CAMERA_SIZE_Y / 2.f), Rgba8::WHITE);
		//g_theRenderer->SetDepthMode()
		//g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->Draw(3, 0);
		g_theRenderer->BindTexture(nullptr, 2);

		// Merge
		g_theRenderer->ClearRenderTarget(Rgba8(0, 0, 0, 0), m_finalTexture);
		g_theRenderer->SetDepthTexture(nullptr, m_finalTexture);
		//g_theRenderer->UnbindDepthStencil();
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->BindShader(m_fluidMerge);
		//g_theRenderer->BindTexture()
		//g_theRenderer->BindTexture(m_depthFinalTexture, 2);
		g_theRenderer->BindTexture(m_depthFinalTexture, 2);
		g_theRenderer->BindTexture(m_thicknessBlurredTexture, 3);
		g_theRenderer->BindTexture(m_normalTexture, 4);
		g_theRenderer->BindTexture(m_backgroundTexture, 5);
		g_theRenderer->BindTexture(m_skyCubeTexture, 6);
		g_theRenderer->Draw(3, 0);
		g_theRenderer->BindTexture(nullptr, 2);
		g_theRenderer->BindTexture(nullptr, 3);
		g_theRenderer->BindTexture(nullptr, 4);
		g_theRenderer->BindTexture(nullptr, 5);
		g_theRenderer->BindTexture(nullptr, 6);

		//g_theRenderer->VSSetSRV(m_depthTexture, 2);

		g_theRenderer->BindShader(nullptr);
		g_theRenderer->VSSetSRV((StructuredBuffer*)nullptr, m_particleSlot);
		g_theRenderer->VSSetSRV((StructuredBuffer*)nullptr, m_densitySlot);
		g_theRenderer->GSSetShader(nullptr);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetDepthTexture(nullptr);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetDepthMode(DepthMode::ENABLED);
		
	}
	//g_theRenderer->DrawVertexBuffer(m_gpuMesh, m_vertexCount);

	//g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
}

void FluidSolver::RenderScreenSpace() const
{
	g_theRenderer->SetModelConstants();

	// Render full screen image
	switch (m_currentView)
	{
	case SV_NONE:
		break;
	case SV_DEPTH: {
		g_theRenderer->BindTexture(m_depthTexture);
		g_theRenderer->BindShader(g_theGame->m_shader);
		break;
	}
	case SV_BLURRED_DEPTH: {
		g_theRenderer->BindTexture(m_depthFinalTexture);
		g_theRenderer->BindShader(g_theGame->m_shader);
		break;
	}
	case SV_THICKNESS: {
		g_theRenderer->BindTexture(m_thicknessTexture);
		break;
	}
	case SV_BLURRED_THICKNESS: {
		g_theRenderer->BindTexture(m_thicknessBlurredTexture);
		break;
	}
	case SV_NORMAL_MAP: {
		g_theRenderer->BindTexture(m_normalTexture);
		break;
	}
	case SV_FINAL_MERGED: {
		g_theRenderer->BindTexture(m_finalTexture);
		break;
	}
	case SV_BACKGROUND: {
		g_theRenderer->BindTexture(m_backgroundTexture);
		break;
	}
	default:
		break;
	}

	DrawSquare(Vec2(0.f, 0.f), Vec2(400.f, 200.f) * 4.f, Rgba8::WHITE);
	//DrawSquare(Vec2(1200.f, 0.f), Vec2(1600.f, 200.f), Rgba8::WHITE);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);

	// Render thickness texture
	if (m_miniThickness == SV_THICKNESS) {
		g_theRenderer->BindTexture(m_thicknessTexture);
		DrawSquare(Vec2(400.0f, 000.f), Vec2(800.f, 200.f), Rgba8::WHITE);
	}
	else if (m_miniThickness == SV_BLURRED_THICKNESS) {
		g_theRenderer->BindTexture(m_thicknessBlurredTexture);
		DrawSquare(Vec2(400.0f, 000.f), Vec2(800.f, 200.f), Rgba8::WHITE);
	}
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(nullptr);

	// Render Depth texture
	//g_theRenderer->BindTexture(m_depthFinalTexture);
	if (m_miniDepth == SV_DEPTH)
		g_theRenderer->BindTexture(m_depthTexture);
	else if (m_miniDepth == SV_BLURRED_DEPTH)
		g_theRenderer->BindTexture(m_depthFinalTexture);
	g_theRenderer->BindShader(g_theGame->m_shader);
	//DrawSquare(Vec2(0.f, 0.f), Vec2(400.f, 200.f) * 4.f, Rgba8::WHITE);
	DrawSquare(Vec2(0.f, 0.f), Vec2(400.f, 200.f), Rgba8::WHITE);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(nullptr);

	// Render normal texture
	if (m_miniNormal == SV_NORMAL_MAP) {
		g_theRenderer->BindTexture(m_normalTexture);
		DrawSquare(Vec2(800.f, 0.f), Vec2(1200.f, 200.f), Rgba8::WHITE);
	}
	g_theRenderer->BindTexture(nullptr);

	g_theRenderer->BindTexture(m_finalTexture);
	//DrawSquare(Vec2(0.f, 0.f), Vec2(400.f, 200.f) * 4.f, Rgba8::WHITE);
	//DrawSquare(Vec2(1200.f, 0.f), Vec2(1600.f, 200.f), Rgba8::WHITE);
	g_theRenderer->BindTexture(nullptr);
}

void FluidSolver::RenderSurfaceInfoFromLight() const
{
	g_theRenderer->EndCamera(g_theGame->m_player->m_camera);
	g_theRenderer->BeginCamera(m_lightCamera);
	g_theRenderer->UnbindDepthStencil();
	g_theRenderer->ClearScreen(Rgba8(0, 0, 0, 0), m_lightDepthTexture);
	g_theRenderer->SetDepthTexture(m_lightDepthTexture);
	g_theRenderer->VSSetShader(m_renderShader);
	g_theRenderer->GSSetShader(m_renderShader);
	g_theRenderer->PSSetShader(m_renderShader);

	//g_theRenderer->VSBindConstantBuffer(k_renderConstantsSlot, m_renderCBO);
	//g_theRenderer->VSBindConstantBuffer(k_simConstantsSlot, m_simCBO);
	//g_theRenderer->GSBindConstantBuffer(k_renderConstantsSlot, m_renderCBO);
	//g_theRenderer->PSBindConstantBuffer(k_renderConstantsSlot, m_renderCBO);

	g_theRenderer->VSSetSRV(m_particleBuffer, m_particleSlot);
	g_theRenderer->VSSetSRV(m_densityBuffer, m_densitySlot);

	g_theRenderer->IASetInputLayout(m_renderShader);
	g_theRenderer->IASetVertexBuffers(nullptr);
	//g_theRenderer->IASetPrimitiveTopologyToPointList();
	g_theRenderer->IASetPrimitiveTopology(Topology::PointList);
	g_theRenderer->BindTexture(m_texture);

	g_theRenderer->Draw(m_fluidNumOfParticles, 0);

	// blur the depth image
	g_theRenderer->CSSetShader(m_bilateralFilterCS);
	SetBlurCBO(Vec2());

	g_theRenderer->SetDepthTexture(nullptr);

	unsigned int dimX = m_depthTexture->GetDimensions().x / 32 + 1;
	unsigned int dimY = m_depthTexture->GetDimensions().y / 32 + 1;

	for (int i = 0; i < 1; i++)
	{
		//SetBlurCBO(Vec2(), 7.f - float(i));
		g_theRenderer->CSSetSRV(m_lightDepthTexture, 0);
		g_theRenderer->CSSetUAV(m_lightBlurredTexture, 0);
		g_theRenderer->DispatchCS(dimX, dimY, 1);
		g_theRenderer->CSSetSRV((Texture*)nullptr, 0);
		g_theRenderer->CSSetUAV((Texture*)nullptr, 0);

		/*
		g_theRenderer->CSSetSRV(m_depthBlurredTexture, 0);
		g_theRenderer->CSSetUAV(m_depthFinalTexture, 0);
		g_theRenderer->DispatchCS(dimX, dimY, 1);
		g_theRenderer->CSSetSRV((Texture*)nullptr, 0);
		g_theRenderer->CSSetUAV((Texture*)nullptr, 0);*/
	}
	g_theRenderer->CSSetShader(nullptr);

	g_theRenderer->IASetPrimitiveTopology(Topology::TriangleList);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMode(DepthMode::ENABLED);
	g_theRenderer->ClearRenderTarget(Rgba8(0, 0, 0, 0), m_lightNormalTexture);
	g_theRenderer->SetDepthTexture(nullptr, m_lightNormalTexture);
	g_theRenderer->BindShader(m_calculateNormalShader);
	//g_theRenderer->BindTexture(m_depthFinalTexture, 2);
	g_theRenderer->BindTexture(m_lightBlurredTexture, 2);

	//std::vector<Vertex_PCU> billboard;

	//DrawSquare(Vec2(-SCREEN_CAMERA_SIZE_X / 2.f, -SCREEN_CAMERA_SIZE_Y / 2.f), Vec2(SCREEN_CAMERA_SIZE_X / 2.f, SCREEN_CAMERA_SIZE_Y / 2.f), Rgba8::WHITE);
	//g_theRenderer->SetDepthMode()
	//g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->Draw(3, 0);
	g_theRenderer->BindTexture(nullptr, 2);

	g_theRenderer->SetDepthTexture(nullptr);

	g_theRenderer->CSSetShader(m_calculateCausticCS);
	g_theRenderer->CSSetSRV(m_lightBlurredTexture, 2);
	g_theRenderer->CSSetSRV(m_lightNormalTexture, 3);
	g_theRenderer->CSSetUAV(m_causticsProjectPosTexture, 0);

	g_theRenderer->DispatchCS(dimX, dimY, 1);
	g_theRenderer->CSSetSRV((Texture*)nullptr, 2);
	g_theRenderer->CSSetSRV((Texture*)nullptr, 3);
	g_theRenderer->CSSetUAV((Texture*)nullptr, 0);

	g_theRenderer->CSSetShader(nullptr);

	g_theRenderer->EndCamera(m_lightCamera);
	g_theRenderer->BeginCamera(g_theGame->m_player->m_camera);

	g_theRenderer->SetDepthTexture(nullptr, m_backgroundTexture);
	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetBlendMode(BlendMode::ADDITIVE);
	g_theRenderer->VSSetShader(m_generateCaustics);
	g_theRenderer->GSSetShader(m_generateCaustics);
	g_theRenderer->PSSetShader(m_generateCaustics);
	g_theRenderer->VSSetSRV(m_causticsProjectPosTexture, 0);
	g_theRenderer->IASetInputLayout(m_generateCaustics);
	g_theRenderer->IASetVertexBuffers(nullptr);
	//g_theRenderer->IASetPrimitiveTopologyToPointList();
	g_theRenderer->IASetPrimitiveTopology(Topology::PointList);

	g_theRenderer->Draw(m_causticsProjectPosTexture->GetDimensions().x * m_causticsProjectPosTexture->GetDimensions().y, 0);

	g_theRenderer->SetDepthTexture(nullptr, nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->IASetPrimitiveTopology(Topology::TriangleList);
	g_theRenderer->VSSetSRV((Texture*)nullptr, 0);
	g_theRenderer->VSSetShader(nullptr);
	g_theRenderer->GSSetShader(nullptr);
	g_theRenderer->PSSetShader(nullptr);
}

void FluidSolver::RenderSandPlane() const
{
	g_theRenderer->SetDepthTexture(nullptr, m_backgroundTexture);
	g_theRenderer->BindTexture(m_sandDiffuse);
	g_theRenderer->BindTexture(m_sandNormal, 1);
	SetSandLightCBO();
	std::vector<Vertex_PCUTBN> sandPlane;
	//AddVertsForQuad3D_PNCU(sandPlane, Vec3(-4.f, 10.f, -1.f), Vec3(-4.f, -4.f, -1.f), Vec3(10.f, -4.f, -1.f), Vec3(10.f, 10.f, -1.f), Rgba8::WHITE);
	AddVertsForQuad3D_PNCU(sandPlane, Vec3(-2.f, 8.f, -1.f), Vec3(-2.f, -2.f, -1.f), Vec3(8.f, -2.f, -1.f), Vec3(8.f, 8.f, -1.f), Rgba8::WHITE);
	g_theRenderer->DrawVertexArray((int)sandPlane.size(), sandPlane.data());
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindTexture(nullptr, 1);
}

void FluidSolver::RenderSkybox() const
{
	g_theRenderer->ClearRenderTarget(Rgba8(0, 0, 0, 0), m_backgroundTexture);
	g_theRenderer->SetDepthTexture(nullptr, m_backgroundTexture);
	g_theRenderer->SetSamplerMode(SamplerMode::BILINEAR_WRAP);
	g_theRenderer->SetModelConstants(GetSkyboxMatrix());
	g_theRenderer->BindTexture(m_skyCubeTexture);
	g_theRenderer->BindShader(m_skyboxShader);
	g_theRenderer->DrawVertexIndexBuffer(m_skyboxVBO, m_skyboxIBO, m_skyboxIBO->m_size / sizeof(unsigned int));
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->SetModelConstants();
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
}

void FluidSolver::SetSimulatorCBO() const
{
	SimulatorConstants simConstants = {};
	simConstants.m_deltaSeconds = m_clock->GetDeltaSeconds();
	if (m_clock->GetDeltaSeconds() > m_timeStep)
		simConstants.m_deltaSeconds = m_timeStep;
	if (simConstants.m_deltaSeconds < 1e-3f) {
		simConstants.m_deltaSeconds = m_timeStep;
	}
	simConstants.m_gravity = 9.8f;
	simConstants.m_particleRadius = m_particleRadius;
	simConstants.m_kernelRadius = m_sphRadius;
	simConstants.m_density0 = m_density0;
	simConstants.m_numOfFluidParticles = (int)m_density.size();
	simConstants.m_numOfParticles = (int)m_particles.size();
	simConstants.m_viscosity = m_viscosity;
	
	simConstants.m_externalForce = m_externalForce;
	if (simConstants.m_externalForce == Vec3()) {
		//float scaler = SinDegrees(m_clock->GetTotalSeconds() * 50.f) * 5.f > 0.f? SinDegrees(m_clock->GetTotalSeconds() * 50.f) * 5.f : 0.f;
		//simConstants.m_externalForce = Vec3(scaler, 0.f, 0.f);
	}
	simConstants.m_scorrK = m_scorrK;
	simConstants.m_scorrQ = m_scorrQ;
	g_theRenderer->CopyCPUToGPU(&simConstants, sizeof(SimulatorConstants), g_theGame->m_simCBO);
	g_theRenderer->CSBindConstantBuffer(k_simConstantsSlot, g_theGame->m_simCBO);
	g_theRenderer->BindConstantBuffer(k_simConstantsSlot, g_theGame->m_simCBO);
}

void FluidSolver::SetRenderCBO() const
{
	RenderConstants renderConstants = {};
	renderConstants.m_eyePos = g_theGame->m_player->m_position;
	renderConstants.m_sphereRadius = m_particleRadius;
	renderConstants.m_depthDimenstion = Vec2((float)m_depthTexture->GetDimensions().x, (float)m_depthTexture->GetDimensions().y);
	Vec3 i, j, k;
	g_theGame->m_player->m_orientation.GetVectors_XFwd_YLeft_ZUp(i, j, k);
	renderConstants.m_eyeUp = Vec4(k);
	renderConstants.m_eyeLeft = Vec4(j);
	Rgba8 waterColor = Rgba8(0, 100, 255, 255);
	waterColor.GetAsFloats(renderConstants.m_waterColor);
	g_theRenderer->CopyCPUToGPU(&renderConstants, sizeof(RenderConstants), g_theGame->m_renderCBO);
	g_theRenderer->BindConstantBuffer(k_renderConstantsSlot, g_theGame->m_renderCBO);
}

void FluidSolver::SetFluidLightCBO() const
{
	FluidLightConstants fluidLightConstants = {};
	fluidLightConstants.m_lightDirt = Vec3(1.f, 1.f, -1.f);
	fluidLightConstants.m_lightPos = Vec3(5.f, 5.f, 25.f);
	fluidLightConstants.m_shininess = 200.f;
	fluidLightConstants.m_fresPower = 0.f;
	fluidLightConstants.m_fresScale = 0.1f;
	fluidLightConstants.m_fresBias = 0.1f;

	fluidLightConstants.m_InvSkyboxModelMat = GetSkyboxMatrix().GetInversedMatrix();

	g_theRenderer->CopyCPUToGPU(&fluidLightConstants, sizeof(FluidLightConstants), g_theGame->m_lightCBO);
	g_theRenderer->BindConstantBuffer(k_fluidLightConstantsSlot, g_theGame->m_lightCBO);
}

void FluidSolver::SetSandLightCBO() const
{
	FluidLightConstants fluidLightConstants = {};
	fluidLightConstants.m_lightDirt = g_theGame->m_player->m_position;
	fluidLightConstants.m_lightPos = m_lightCamera.m_position;
		//Vec3(5.f, 5.f, 25.f);
	fluidLightConstants.m_shininess = 200.f;
	fluidLightConstants.m_fresPower = 0.f;
	fluidLightConstants.m_fresScale = 0.1f;
	fluidLightConstants.m_fresBias = 0.1f;
	fluidLightConstants.m_specularIntensity = 100.f;
	fluidLightConstants.m_lightIntensity = 1000.f;
	fluidLightConstants.m_ambientIntensity = Vec4(100.f, 0.f, 0.f, 0.f);

	fluidLightConstants.m_InvSkyboxModelMat = GetSkyboxMatrix().GetInversedMatrix();

	g_theRenderer->CopyCPUToGPU(&fluidLightConstants, sizeof(FluidLightConstants), g_theGame->m_lightCBO);
	g_theRenderer->BindConstantBuffer(k_fluidLightConstantsSlot, g_theGame->m_lightCBO);
}

void FluidSolver::SetBlurCBO(Vec2 dir, float sigmaD) const
{
	BlurConstants blurConstant;
	blurConstant.FilterRadius = 6.f;
	blurConstant.BlurDepthFalloff = 10.f;
	blurConstant.BlurScale = 0.001f;
	blurConstant.BlurDirt = dir;

	blurConstant.ImageHeight = g_theWindow->GetClientDimensions().y;
	blurConstant.ImageWidth = g_theWindow->GetClientDimensions().x;
	blurConstant.SigmaD = sigmaD;
	blurConstant.SigmaR = 0.0008f;

	blurConstant.Padding0 = m_lightCamera.m_position;
	
	g_theRenderer->CopyCPUToGPU(&blurConstant, sizeof(BlurConstants), g_theGame->m_blurCBO);
	g_theRenderer->BindConstantBuffer(k_blurConstantsSlot, g_theGame->m_blurCBO);
	g_theRenderer->CSBindConstantBuffer(k_blurConstantsSlot, g_theGame->m_blurCBO);

}

void FluidSolver::ApplyBilateralFilter() const
{
	g_theRenderer->CSSetShader(m_bilateralFilterCS);
	SetBlurCBO(Vec2());

	unsigned int dimX = m_depthTexture->GetDimensions().x / 32 + 1 ;
	unsigned int dimY = m_depthTexture->GetDimensions().y / 32 + 1 ;

	for (int i = 0; i < 1; i++)
	{
		//SetBlurCBO(Vec2(), 7.f - float(i));
		g_theRenderer->CSSetSRV(m_depthTexture, 0);
		g_theRenderer->CSSetUAV(m_depthBlurredTexture, 0);
		g_theRenderer->DispatchCS(dimX, dimY, 1);
		g_theRenderer->CSSetSRV((Texture*)nullptr, 0);
		g_theRenderer->CSSetUAV((Texture*)nullptr, 0);

		g_theRenderer->CSSetSRV(m_depthBlurredTexture, 0);
		g_theRenderer->CSSetUAV(m_depthFinalTexture, 0);
		g_theRenderer->DispatchCS(dimX, dimY, 1);
		g_theRenderer->CSSetSRV((Texture*)nullptr, 0);
		g_theRenderer->CSSetUAV((Texture*)nullptr, 0);
	}
	/*
	g_theRenderer->CSSetSRV((Texture*)nullptr, 0);
	g_theRenderer->CSSetUAV((Texture*)nullptr, 0);
	g_theRenderer->CSSetSRV(m_thicknessTexture, 0);
	g_theRenderer->CSSetUAV(m_thicknessBlurredTexture, 0);
	g_theRenderer->DispatchCS(dimX, dimY, 1);
	g_theRenderer->CSSetSRV((Texture*)nullptr, 0);
	g_theRenderer->CSSetUAV((Texture*)nullptr, 0);
	*/
	g_theRenderer->CSSetShader(nullptr);
	
}

Mat44 FluidSolver::GetSkyboxMatrix() const
{
	Mat44 res;
	EulerAngles orientation;
	res = orientation.GetMatrix_XFwd_YLeft_ZUp();
	res.SetTranslation3D(g_theGame->m_player->m_position);
	res.AppendScaleUniform3D(1.0f);
	return res;
}
