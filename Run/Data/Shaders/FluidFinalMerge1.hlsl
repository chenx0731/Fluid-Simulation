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
    float4 color : SV_TARGET0;
   float depth : SV_DEPTH;
};

cbuffer LightConstants : register(b1)
{
    float3 LightDir;
    float  Shininess;

    float3 LightPos;
    float  FresPower;

    float  FresScale;
    float  FresBias;
    float  Padding0;
    float  Padding1;

    float4x4 InvSkyboxModelMatrix;
}

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
    float Padding2;
    float Padding3;
    float4 WaterColor;
}
//------------------------------------------------------------------------------------------------
Texture2D diffuseTexture : register(t0);
StructuredBuffer<Particle> ParticlesRO : register(t1);
Texture2D depthTexture : register(t2);
Texture2D thicknessTexture : register(t3);
Texture2D normalTexture : register(t4);
Texture2D backgroundTexture : register(t5);
TextureCube skyboxTexture : register(t6);
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
	v2p.color = WaterColor;
	v2p.uv = FS_UVS[input.vidx];
	return v2p;
}

float3 reflect(float3 light, float3 normal)
{
    return - 2 * dot(light, normal) * normal + light;
}

PSOutput PixelMain(VSOutput input)
{
    PSOutput OUT;   
    float3 normal = normalTexture.Sample(diffuseSampler, input.uv).xyz;
    float depth = depthTexture.Sample(diffuseSampler, input.uv).r;
    // get eye space position
    float3 eyeSpacePos = uvToEye(input.uv, depth);
    float3 worldPos = mul(InvViewMatrix, float4(eyeSpacePos, 1.f)).xyz;
    float4 scene = backgroundTexture.Sample(diffuseSampler, input.uv);
    float3 lightPosInView = mul(ViewMatrix, float4(LightPos, 1.f)).xyz;
    // dist to the light position
    float r = length(lightPosInView - eyeSpacePos);
    r = r * r;
    // cubemap reflecting
    float3 reflectV = reflect(eyeSpacePos, normal);
    //reflectV = normalize(reflectV);
    float4 clipV =  mul(ProjectionMatrix, float4(reflectV, 0.f));
    clipV.w = clipV.z;
    float4 reflectVP = mul(InvProjectionMatrix, clipV);
    float4 worldRP = mul(InvViewMatrix, reflectVP);
    
    //float4 clip
    float3 worldReflect =  mul(InvViewMatrix, float4(reflectV, 0.f)).xyz;
   // float3 modelReflect = mul(InvSkyboxModelMatrix, float)
    worldReflect = normalize(worldReflect);
    float3 erColor = skyboxTexture.Sample(diffuseSampler, worldReflect).xyz;
    
/*
    float thickness = max(thicknessTexture.Sample(diffuseSampler, input.uv).r, 0.3f);
    float3 transmission = exp(-(1.f - input.color.xyz) * thickness);

    float refractScale = 1.33 * 0.025;
    refractScale *= smoothstep(0.1, 0.4, worldPos.z);
*/

    //Phong Sepcular
    float3 light = lightPosInView - eyeSpacePos;
//=
//mul(ViewMatrix, float4(LightDir, 0.f)).xyz;
    float3 viewDirt = - normalize(eyeSpacePos);
    float3 viewDirInView = -viewDirt;
    viewDirt = normalize(mul(ViewMatrix, float4(viewDirt, 0.f)).xyz);
    float3 halfVec = normalize(viewDirt + light);

    float specular = pow(max(0.f, dot(normal, halfVec)), Shininess)* (100.f / r);
      //vec3(dirLight.specular * pow(max(dot(halfVec, normal), 0.0f), 400.0f));
    
    float2 texScale = float2(1.f, 1.f) * 0.8f;
    float refractScale = 1.33 * 0.025;
    refractScale *= smoothstep(0.1, 0.4, worldPos.z);
    float2 refractCoord = input.uv + normal.xy * refractScale * texScale;

    float thickness = clamp(max(thicknessTexture.Sample(diffuseSampler, input.uv).r, 0.2f), 0.f, 1.f);
    float3 transmission = exp(-(1.f - input.color.xyz) * thickness);

    float3 refract = backgroundTexture.Sample(diffuseSampler, refractCoord).xyz * transmission;

    float3 lVec = normalize(worldPos.xyz - LightPos);
    float attenuation = max(smoothstep(0.95, 1.0, abs(dot(lVec, -LightDir))), 0.05);

    float ln = dot(light, normal) * attenuation;
    
    float3 diffuse = input.color.xyz * max(dot(light, normal) * 0.5f + 0.5f, 0.f) * (10.f / r) * input.color.w;
    
    float r0 = 0.2f;
    
    float fresPower = 100;
    
    float fresnel = r0 + (1 - r0) * pow(1 - max(dot(viewDirInView, normal) * 0.5f + 0.5f, 0.f), fresPower);

    float3 finalColor = diffuse * 0.4f  + (specular.xxx +lerp(refract, erColor, fresnel)) * input.color.w;
// specular.xxx + refract;
    //float3 finalColor = diffuse * 0.4f + (specular.xxx + refract + erColor * 0.4f) * input.color.w;
    
    OUT.color = float4(finalColor, 1.f);
//float4(1.f, 0.f, 0.f, 1.f);
//float4(finalColor, 1.f);
    OUT.depth = depth;
    if (depth == 1.f)
    OUT.color = scene;
//depth;

    return OUT;
}
