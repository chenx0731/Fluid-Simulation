#include "Constants.hlsl"
//-----------------------------------------------------------------------------------------------

struct VSInput
{
    uint vidx : SV_VERTEXID;
};
static float3 FS_POS[3] =
{
    float3(-1.0f, -1.0f, 0.0f),
  
    float3(3.0f, -1.0f, 0.0f),

    float3(-1.0f, 3.0f, 0.0f),
};

static float2 FS_UVS[3] =
{
    float2(0.0f, 1.0f),
    float2(2.0f, 1.0f),
    float2(0.0f, -1.0f),
};
//------------------------------------------------------------------------------------------------
struct VSOutput
{
	float4 position : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};
struct PSOutput
{
    float4 color : SV_TARGET;
    //float depth : SV_DEPTH;
};
//------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b2)
{
	float4x4 ProjectionMatrix;
	float4x4 ViewMatrix;
    float4x4 InvProjectionMatrix;
    float4x4 InvViewMatrix;
};

//------------------------------------------------------------------------------------------------
cbuffer ModelConstants : register(b3)
{
	float4x4 ModelMatrix;
	float4 ModelColor;
};

cbuffer RenderConstants : register(b4)
{
    float3 EyeSpacePos;
    float SphereRadius;
    float4 CameraUpDir;
    float4 CameraLeftDir;
    float2 DepthTextureSize;
    float Padding0;
    float Padding1;
}
//------------------------------------------------------------------------------------------------
Texture2D diffuseTexture : register(t0);
StructuredBuffer<Particle> ParticlesRO : register(t1);
Texture2D depthTexture : register(t2);
Texture2D normalTexture : register(t3);
Texture2D backgroundTexture : register(t4);
//------------------------------------------------------------------------------------------------
SamplerState diffuseSampler : register(s0);

float3 uvToEye(float2 coord, float z)
{
    float2 pos = coord * 2.f - 1.f;
    float4 clipPos = float4(pos.x, -pos.y, z, 1.f);
    float4 viewPos = mul(InvProjectionMatrix, clipPos);
    return (viewPos / viewPos.w).xyz;
}

//------------------------------------------------------------------------------------------------
VSOutput VertexMain(VSInput input)
{
	VSOutput v2p;
    v2p.position = float4(FS_POS[input.vidx], 1.f);
	v2p.color = 0.f.xxxx;
	v2p.uv = FS_UVS[input.vidx];
	return v2p;
}

PSOutput PixelMain(VSOutput input)
{
    PSOutput OUT;    
    float2 depthTexelSize = 1.f / DepthTextureSize;
    float2 depthDx = float2(depthTexelSize.x, 0.f);
    float2 depthDy = float2(0.f, depthTexelSize.y);
    float depth = depthTexture.Sample(diffuseSampler, input.uv).r;
    // get eye space position
    float3 eyeSpacePos = uvToEye(input.uv, depth);
    
    // finite difference
    // left
    float  depthDxLeft = depthTexture.Sample(diffuseSampler, input.uv - depthDx).r;
    float3 ddxLeft = eyeSpacePos - uvToEye(input.uv - depthDx, depthDxLeft);
    // right
    float  depthDxRight = depthTexture.Sample(diffuseSampler, input.uv + depthDx).r;
    float3 ddxRight = uvToEye(input.uv + depthDx, depthDxRight) - eyeSpacePos;
    // up
    float  depthDyUp = depthTexture.Sample(diffuseSampler, input.uv - depthDy).r;
    float3 ddyUp = uvToEye(input.uv - depthDy, depthDyUp) - eyeSpacePos;
    //ddyUp.z = abs(ddyUp.z);
    // down
    float  depthDyDown = depthTexture.Sample(diffuseSampler, input.uv + depthDy).r;
    float3 ddyDown = eyeSpacePos - uvToEye(input.uv + depthDy, depthDyDown);
    //ddyDown.z = abs(ddyDown.z);
    
    float3 dx = ddxLeft;
    float3 dy = ddyUp;
    if (abs(ddxRight.z) < abs(ddxLeft.z))
        dx = ddxRight;
    if (abs(ddyDown.z) < abs(ddyUp.z))
        dy = ddyDown;
    float3 normal = normalize(cross(dx, dy));
    OUT.color = float4(normal, 1.f);
    float3 worldNormal = mul(InvViewMatrix, float4(normal, 0.f)).xyz;
    normalize(worldNormal);
    OUT.color =  float4(worldNormal, 1.f);
    //float3 worldPos = (InvViewMatrix * float4(eyeSpacePos, 1.f)).xyz;
    return OUT;
}
