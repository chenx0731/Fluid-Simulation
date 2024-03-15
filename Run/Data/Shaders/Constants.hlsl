struct Particle
{
	float Mass;
	float InvMass;
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
    float ParticleRadius;
    float KernelRadius;

    float StaticDensity;
	uint NumOfFluidParticles;
	uint NumOfParticles;
	bool IsUsingCubicSpline;

	float Viscosity;
	float3 ExternalForce;

	float ScorrK;
	float ScorrQ;
	float padding0;
	float padding1;
};