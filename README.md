[Fluid Simulation - Chen Xu](https://www.chenoa.games/personal-project/fluid-simulation)

---

| ROLE      | Graphics&physics programmer                |

| Engine    | Custom Game Engine                         |

| Dev Time  | Aug 2023 - Present                         |

| Languages | C++, HLSL                                  |

| Tools     | Visual Studio, DirectX11, RenderDoc, ImGui |

Fluid simulation is a challenging problem in terms of physics simulation and realistic rendering. This project focuses on both the simulation and rendering to make the fluid interactive and look satisfying.

For this project, I choose Position Based Fluid as the simulation solver and Screen Space Rendering for real time rendering. And I develop this project on my C++ custom game engine using DirectX11 and DX compute shader for parallelization computing.



## Current Features

- Position based fluid
  - The solver for physics simulation, it's a lagrangian-based system utilizing a Jacobi-style update for compute all particles in parallel on the GPU.

- Screen space rendering
  - Generate depth, thickness image of all particles and get the normal map in view space using depth image, and render the final image by using Blinn-Phong and skybox reflection model and Beer's law for refraction. Reflection and refraction are adjusted by Fresnel parameter.

- Spatial hashing neighbor searching
  - A neighbor searching method for inserting all particles into cells based on the SPH kernel radius, each particle can easily get its neighbors in 27 cells of its local area to compute its density and velocity changed by viscosity.

- Smoothed depth / thickness image
  - Get smoothed depth/thickness image by using bilateral filter to keep the boundary, which makes the surface of water look less particle-like.

![Fluid_CubemapR](D:\MySpace\Site\Fluid\Fluid_CubemapR.gif)