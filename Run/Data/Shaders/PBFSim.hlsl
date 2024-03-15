

#include "Constants.hlsl"
#include "SpatialHash3D.hlsl"
#include "SPHKernels.hlsl"
						

RWStructuredBuffer<Particle> Particles : register(u1);
RWStructuredBuffer<uint3> SpatialIndices : register(u2); // spatial hashing
RWStructuredBuffer<uint> SpatialOffsets : register(u3); // spatial hashing
RWStructuredBuffer<float> Densities : register(u4);
RWStructuredBuffer<float> Lambdas : register(u5);

StructuredBuffer<Particle> ParticlesRO : register(t1);
StructuredBuffer<uint3> SpatialIndicesRO : register(t2); // spatial hashing
StructuredBuffer<uint> SpatialOffsetsRO : register(t3); // spatial hashing
StructuredBuffer<float> DensitiesRO : register(t4);
StructuredBuffer<float> LambdasRO : register(t5);

[numthreads(256, 1, 1)]
void ApplyGravity(uint3 id : SV_DispatchThreadID)
{
	//int arraySize = Particles;
	
	if (id.x < NumOfFluidParticles)
	{
		// if it works fine, then no need to change p back to Particles[threadID.x]
		//Particle p = Particles[threadID.x];
        if (Particles[id.x].Mass != 0.0)
		{
			Particles[id.x].Acceleration = float3(0.f, 0.f, -Gravity);
			Particles[id.x].Acceleration += ExternalForce;
            Particles[id.x].LastPosition = Particles[id.x].OldPosition;
			Particles[id.x].OldPosition = Particles[id.x].Position;
			Particles[id.x].Velocity = Particles[id.x].Velocity + DeltaSeconds * Particles[id.x].Acceleration;
            Particles[id.x].Position = Particles[id.x].Position + DeltaSeconds * Particles[id.x].Velocity;

            //Particles[threadID.x] = p;
        }
	}
	uint index = id.x;
	if (Particles[id.x].Position.x >= 8.f - ParticleRadius){
		Particles[id.x].Position.x = 8.f - ParticleRadius;
		Particles[id.x].Velocity = (Particles[index].Position - Particles[index].OldPosition) / DeltaSeconds;
	}
		//Particles[id.x].Position.x = 8.f - ParticleRadius;
	if (Particles[id.x].Position.x <= -2.f + ParticleRadius){
		Particles[id.x].Position.x = -2.f + ParticleRadius;
		Particles[id.x].Velocity = (Particles[index].Position - Particles[index].OldPosition) / DeltaSeconds;
	}
				

	if (Particles[id.x].Position.y >= 8.f - ParticleRadius) {
		Particles[id.x].Position.y = 8.f - ParticleRadius;
		Particles[id.x].Velocity = (Particles[index].Position - Particles[index].OldPosition) / DeltaSeconds;
	}
	if (Particles[id.x].Position.y <= -2.f + ParticleRadius) {
		Particles[id.x].Position.y = -2.f + ParticleRadius;	
		Particles[id.x].Velocity = (Particles[index].Position - Particles[index].OldPosition) / DeltaSeconds;
	}

	if (Particles[id.x].Position.z >= 15.f - ParticleRadius) {
		Particles[id.x].Position.z = 15.f - ParticleRadius;	
		Particles[id.x].Velocity = (Particles[index].Position - Particles[index].OldPosition) / DeltaSeconds;
	}
	if (Particles[id.x].Position.z <= -1.f + ParticleRadius) {
		Particles[id.x].Position.z = -1.f + ParticleRadius;
		Particles[id.x].Velocity = (Particles[index].Position - Particles[index].OldPosition) / DeltaSeconds;
	}
}

[numthreads(256, 1, 1)]
void UpdateSpatialHash(uint3 id : SV_DispatchThreadID)
{
    //AllMemoryBarrierWithGroupSync();
	if (id.x >= NumOfParticles)
        return;
    else
    {
        float3 pos = ParticlesRO[id.x].Position;
        int3 cell = GetCell3D(pos, KernelRadius);
        uint hash = HashCell3D(cell);
        uint key = KeyFromHash(hash, NumOfParticles);
        int index = id.x;
        SpatialIndices[id.x] = int3(index, hash, key);
    }

}

[numthreads(256, 1, 1)]
void ComputeDensity(uint3 id : SV_DispatchThreadID)
{
	if(id.x >= NumOfFluidParticles)
		return;
	float res = 0.0f;
	uint index = id.x;
	int3 originCell = GetCell3D(ParticlesRO[index].Position, KernelRadius);
	//res += ParticlesRO[index].Mass * Poly6(float3(0.f, 0.f, 0.f), KernelRadius, IsUsingCubicSpline);
	Particle p = ParticlesRO[index];
	[unroll(27)]
	for (int i = 0; i < 27; i++) {
		uint hash = HashCell3D(originCell + offsets3D[i]);
		uint key = KeyFromHash(hash, NumOfParticles);
		uint currIndex = SpatialOffsets[key];
		while (currIndex < NumOfParticles) {
			uint3 indexData = SpatialIndices[currIndex];
			currIndex++;
			// Exit if no longer looking at correct bin
			if (indexData.z != key) break;
			// Skip if hash doesn't match
			if (indexData.y != hash) continue;

			uint neighborIndex = indexData[0];
			Particle np = ParticlesRO[neighborIndex];
			float3 r = p.Position - np.Position;
			res += np.Mass * Poly6(r, KernelRadius, IsUsingCubicSpline);	//
			
		}
	}
	Densities[index] = res;
	//Densities[index] = res;
}

[numthreads(256, 1, 1)]
void ComputeLambda(uint3 id : SV_DispatchThreadID)
{
	if(id.x >= NumOfFluidParticles)
		return;
	int index = id.x;
	float eps = 1.0e-6f;
	float ci = DensitiesRO[index] / StaticDensity - 1.f;
	if (ci < 0.f)
		ci = 0.f;
	if (ci == 0.f) {
		Lambdas[index] = 0.f;
		return;
	}
	float neighborCount = 0.f;
	float sum_gradj = 0.f;
	float3 gradi = float3(0.f, 0.f, 0.f);
	int3 originCell = GetCell3D(ParticlesRO[index].Position, KernelRadius);
	Particle p = ParticlesRO[index];
	float inverseD0 = 1.f / StaticDensity;
	[unroll(27)]
	for (int i = 0; i < 27; i++) {
		uint hash = HashCell3D(originCell + offsets3D[i]);
		uint key = KeyFromHash(hash, NumOfParticles);
		uint currIndex = SpatialOffsets[key];
		while (currIndex < NumOfParticles) {
			uint3 indexData = SpatialIndices[currIndex];
			currIndex++;
			// Exit if no longer looking at correct bin
			if (indexData.z != key) break;
			// Skip if hash doesn't match
			if (indexData.y != hash) continue;

			uint neighborIndex = indexData[0];
			Particle np = ParticlesRO[neighborIndex];
			float3 r = p.Position - np.Position;
			float3 gradj = - SpikyGrad(r, KernelRadius, IsUsingCubicSpline) * (inverseD0 * np.Mass);
			sum_gradj += length(gradj) * length(gradj);
			gradi += gradj;
			neighborCount++;	
		}
	}
	sum_gradj += length(gradi) * length(gradi);
	Lambdas[index] = - ci / (sum_gradj + eps);
}

[numthreads(256, 1, 1)]
void ComputeDeltaPos(uint3 id : SV_DispatchThreadID)
{
	if(id.x >= NumOfFluidParticles)
		return;
	int index = id.x;
	float3 deltaPos = 0.f.xxx;
	Particle p = Particles[index];
	int3 originCell = GetCell3D(p.Position, KernelRadius);
	
	[unroll(27)]
	for (int i = 0; i < 27; i++) {
		uint hash = HashCell3D(originCell + offsets3D[i]);
		uint key = KeyFromHash(hash, NumOfParticles);
		uint currIndex = SpatialOffsets[key];
		while (currIndex < NumOfParticles) {
			uint3 indexData = SpatialIndices[currIndex];
			currIndex++;
			// Exit if no longer looking at correct bin
			if (indexData.z != key) break;
			// Skip if hash doesn't match
			if (indexData.y != hash) continue;

			uint neighborIndex = indexData[0];
			Particle np = Particles[neighborIndex];
			float3 r = p.Position - np.Position;
			if (neighborIndex == id.x)
				continue;
			float3 gradi = SpikyGrad(r, KernelRadius, IsUsingCubicSpline);
			float3 q = float3(1.f, 0.f, 0.f);
			q *= KernelRadius * ScorrQ;
			float scorr = Poly6(r, KernelRadius, IsUsingCubicSpline) / Poly6(q, KernelRadius, IsUsingCubicSpline);
			scorr *= scorr * scorr * scorr;
			scorr = ScorrK * scorr;
			//scorr = 0.f;
			if (neighborIndex < NumOfFluidParticles) {
				deltaPos += (LambdasRO[index] + LambdasRO[neighborIndex] + scorr) * gradi;
//LambdasRO[index] + LambdasRO[neighborIndex] + 
			}
			else deltaPos += (LambdasRO[index]) * gradi;// LambdasRO[index] +
		}
	}
	deltaPos /= (StaticDensity);
	//Particles[index].OldPosition = Particles[index].Position;
	Particles[index].Position += deltaPos;
}

[numthreads(256, 1, 1)]
void UpdateVelocityByPosition(uint3 id : SV_DispatchThreadID)
{
	if(id.x >= NumOfFluidParticles)
		return;
	int index = id.x;
	Particles[index].Velocity = (Particles[index].Position - Particles[index].OldPosition) / DeltaSeconds;
}

[numthreads(256, 1, 1)]
void ApplyVorticityConfinement(uint3 id : SV_DispatchThreadID)
{
	
}

[numthreads(256, 1, 1)]
void ApplyViscosity(uint3 id : SV_DispatchThreadID)
{
	if(id.x >= NumOfFluidParticles)
		return;
	int index = id.x;
	float3 deltaVel = 0.f.xxx;
	Particle p = Particles[index];
	int3 originCell = GetCell3D(p.Position, KernelRadius);
	
	[unroll(27)]
	for (int i = 0; i < 27; i++) {
		uint hash = HashCell3D(originCell + offsets3D[i]);
		uint key = KeyFromHash(hash, NumOfParticles);
		uint currIndex = SpatialOffsets[key];
		while (currIndex < NumOfParticles) {
			uint3 indexData = SpatialIndices[currIndex];
			currIndex++;
			// Exit if no longer looking at correct bin
			if (indexData.z != key) break;
			// Skip if hash doesn't match
			if (indexData.y != hash) continue;

			uint neighborIndex = indexData[0];
			if (neighborIndex >= NumOfFluidParticles)
			Particle np = Particles[neighborIndex];
			
			float3 vel_ij = Particles[neighborIndex].Velocity - Particles[index].Velocity;
			float3 r = Particles[index].Position - Particles[neighborIndex].Position;
			float kernel = Poly6(r, KernelRadius, IsUsingCubicSpline);
			deltaVel += vel_ij * (Viscosity * kernel * (Particles[neighborIndex].Mass / StaticDensity));
		}
	}
	Particles[index].Velocity += deltaVel;
	Particles[index].Position = Particles[index].OldPosition + Particles[index].Velocity * DeltaSeconds;

}