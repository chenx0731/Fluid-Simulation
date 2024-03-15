#include "Constants.hlsl"
/*
struct Particle
{
	double Mass;
	double InvMass;
	float3 Position;
	float3 Velocity;
	float3 LastPosition;
	float3 OldPosition;
	float3 RestPosition;
	float3 Acceleration;
};

cbuffer SimulatorConstants : register(b0)
{
	float DeltaSeconds;
	float Gravity;
	int NumOfFluidParticles;
	int NumOfPaticles;
};*/
RWStructuredBuffer<Particle> Particles : register(u1);

[numthreads(64, 1, 1)]
void ComputeMain(uint3 threadID : SV_DispatchThreadID)
{
	//int arraySize = Particles;
	if (threadID.x < NumOfFluidParticles)
	{
		// if it works fine, then no need to change p back to Particles[threadID.x]
		Particle p = Particles[threadID.x];
		if (p.Mass != 0.0)
		{
			p.Acceleration = float3(0.f, 0.f, -Gravity);
			p.LastPosition = p.OldPosition;
			p.OldPosition = p.Position;
			p.Velocity = p.Velocity + DeltaSeconds * p.Acceleration;
			p.Position = p.Position + DeltaSeconds * p.Velocity;
            Particles[threadID.x] = p;
        }
	}
}