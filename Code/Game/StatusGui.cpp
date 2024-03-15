#include "Game/StatusGui.hpp"
#include "Game/SPHKernels.hpp"
#include "Game/FluidSolver.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "ThirdParty/ImGui/imgui.h"
#include "ThirdParty/ImGui/imgui_impl_dx11.h"
#include "ThirdParty/ImGui/imgui_impl_win32.h"
#include <d3d11.h>

StatusGui::StatusGui(Game* game, ImGuiBaseConfig config) : ImGuiBase(config), m_game(game)
{
}

void StatusGui::Update()
{
    ImGui::Begin("Physics");
    ImGui::Text("Press Q & E to add external forces");
    //ImGui::InputDouble("Radius", &(m_game->m_fluidSolver->m_particleRadius), 0.0, 0.1);
    float minR = 0.01f;
    float maxR = 0.2f;
    float minSPH = 0.175f;
    float maxSPH = 0.5f;
    float minVisc = 0.001f;
    float maxVisc = 1.f;
    float minK = -0.1f;
    float maxK = -0.000f;
    float minQ = 0.001f;
    float maxQ = 0.3f;
    float minT = 0.005f;
    float maxT = 0.1f;
	ImGui::DragFloat("Radius", &(m_game->m_fluidSolver->m_particleRadius), 0.001f, minR, maxR);
    ImGui::DragFloat("SPHRadius", &(m_game->m_fluidSolver->m_sphRadius), 0.001f, minSPH, maxSPH);
    ImGui::DragFloat("Viscosity", &(m_game->m_fluidSolver->m_viscosity), 0.001f, minVisc, maxVisc);
    ImGui::DragFloat("Scorr K", &(m_game->m_fluidSolver->m_scorrK), 0.0001f, minK, maxK);
    ImGui::DragFloat("Scorr Q", &(m_game->m_fluidSolver->m_scorrQ), 0.001f, minQ, maxQ);
    ImGui::DragFloat("Time Step", &(m_game->m_fluidSolver->m_timeStep), 0.001f, minT, maxT);
    ImGui::Checkbox("Apply Pressure", &m_game->m_fluidSolver->m_isApplyingPressure);
    ImGui::Checkbox("Apply Viscosity", &m_game->m_fluidSolver->m_isApplyingViscosity);
    ImGui::Checkbox("Cubic Spline", &SPHKernel::m_isUsingCubicSpline);
    if (ImGui::Button("Reset Simulation")) {
        m_game->m_fluidSolver->ResetSimulation();
    }
    //ImGui::DragScalar("Time Step", ImGuiDataType_Double, &(m_game->m_fluidSolver->m_timeStep), 0.001f, &min, &max);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", m_game->m_clock->GetDeltaSeconds() * 1000.f, 1.f / m_game->m_clock->GetDeltaSeconds());
    //ImGui::DragFloat("TimeStep", )
    ImGui::End();
    //ImGui::DragFloat("Radius", &(m_game->m_fluidSolver->m_particleRadius), );

    //ImGui::Begin("Render");
    //ImGui::Checkbox("Full Screen Depth Image", &m_game->m_fluidSolver->m_isApplyingPressure);

    ImGui::Begin("Render");
	const char* screenViews[] = {"Depth", "Blurred Depth", "Thickness", "Blurred Thickness", "Normal Map", "Final Merged", "Background"};
	static const char* item_current = screenViews[0];            // Here our selection is a single pointer stored outside the object.
    ImGui::Combo("Image", (int *)&(m_game->m_fluidSolver->m_currentView), screenViews, IM_ARRAYSIZE(screenViews));
    
	ImGui::RadioButton("No Depth", (int*)&(m_game->m_fluidSolver->m_miniDepth), -1); ImGui::SameLine();
	ImGui::RadioButton("Depth", (int*)&(m_game->m_fluidSolver->m_miniDepth), 0); ImGui::SameLine();
	ImGui::RadioButton("Blurred Depth", (int*)&(m_game->m_fluidSolver->m_miniDepth), 1);

	ImGui::RadioButton("No Thick", (int*)&(m_game->m_fluidSolver->m_miniThickness), -1); ImGui::SameLine();
	ImGui::RadioButton("Thickness", (int*)&(m_game->m_fluidSolver->m_miniThickness), 2); ImGui::SameLine();
	ImGui::RadioButton("Blurred Thickness", (int*)&(m_game->m_fluidSolver->m_miniThickness), 3);

	ImGui::RadioButton("No Normal", (int*)&(m_game->m_fluidSolver->m_miniNormal), -1); ImGui::SameLine();
	ImGui::RadioButton("Normal Map", (int*)&(m_game->m_fluidSolver->m_miniNormal), 4);

    ImGui::End();

}
