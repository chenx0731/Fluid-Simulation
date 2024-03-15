#include "Constants.hlsl"
//-----------------------------------------------------------------------------------------------

struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
    float size : TEXCOORD1;
};

struct GSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
    float size : TEXCOORD1;
};

struct PSOutput
{
    float4 color : SV_TARGET;
    //float depth : SV_DEPTH;
};
/*
struct PSOutput
{
    float4 color : SV_TARGET;
    //float depth : SV_DEPTH;
};*/
//------------------------------------------------------------------------------------------------
cbuffer CameraConstants : register(b2)
{
	float4x4 ProjectionMatrix;
	float4x4 ViewMatrix;
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

}
//------------------------------------------------------------------------------------------------
Texture2D diffuseTexture : register(t0);
StructuredBuffer<Particle> ParticlesRO : register(t1);
Texture2D depthTexture : register(t2);
StructuredBuffer<float> DensityRO : register(t4);
//------------------------------------------------------------------------------------------------
SamplerState diffuseSampler : register(s0);


//------------------------------------------------------------------------------------------------
VSOutput VertexMain(uint ID : SV_VertexID)
{
    VSOutput OUT = (VSOutput)0;
    OUT.position = float4(ParticlesRO[ID].Position.xyz, 1.f);
    float distanceToCamera = length(EyeSpacePos - OUT.position.xyz);
    //OUT.size = 0.2f / distanceToCamera;
    OUT.size = 
//clamp(SphereRadius * 0.80f, SphereRadius, SphereRadius * (DensityRO[ID] / 1000.f));
////smoothstep(SphereRadius * 0.7f, SphereRadius, DensityRO[ID] / 1000.f);
SphereRadius;
    //if (DensityRO[ID] < 400.f)
        //OUT.size = SphereRadius * (DensityRO[ID] / 1000.f);
    return OUT;
}

static const float2 g_texcoords[4] = { float2(1, 0), float2(1, 1), float2(0, 0), float2(0, 1) };

[maxvertexcount(4)]
void GeometryMain(point VSOutput In[1], inout TriangleStream<GSOutput> SpriteStream)
{
     //
    // Compute the local coordinate system of the sprite relative to the world
    // space such that the billboard is aligned with the y-axis and faces the eye.
    //

    float3 up = CameraUpDir.xyz;
    //float3(0.0f, 0.0f, 1.0f);
    float3 look = EyeSpacePos - In[0].position.xyz;
    //look.z = 0.f; // project to xy-plane
    look = normalize(look);
    float3 right = cross(up, look);
   
    
    float size = In[0].size;
    
    float3 v[4];
    v[0] = In[0].position.xyz + size * right - size * up;
    v[1] = In[0].position.xyz + size * right + size * up;
    v[2] = In[0].position.xyz - size * right - size * up;
    v[3] = In[0].position.xyz - size * right + size * up;
    
    GSOutput gsout;
    [unroll]
    for (int i = 0; i < 4; i++)
    {    
        float4 pos = float4(v[i], 1.f);
        float4 viewPosition = mul(ViewMatrix, pos);
        float4 clipPosition = mul(ProjectionMatrix, viewPosition);
        gsout.position = clipPosition;
        gsout.color = float4(1.f, 1.f, 1.f, 1.f);

        gsout.texCoord = g_texcoords[i];
        gsout.size = size;
        SpriteStream.Append(gsout);
    }
    SpriteStream.RestartStrip();
}


PSOutput PixelMain(GSOutput input)
{
    PSOutput OUT;
	
	// calculate eye-space sphere normal from texture coordinates
    /*
    float3 N;
    N.xy = input.texCoord * 2.0 - 1.0;
    float r2 = dot(N.xy, N.xy);
    if (r2 > 1.0)
        discard;
    N.z = sqrt(1.0 - r2);
    */
    float3 N;
    N.yz = input.texCoord * 2.0 - 1.0;
    N.y = -N.y;
    float r2 = dot(N.yz, N.yz);
    if (r2 > 1.0)
        discard;
    N.x = -sqrt(1.0 - r2);
    N.x = -1.f;    

    //OUT.color.yzw = OUT.color.xxx;
    OUT.color = float4(-N.x * 0.05f, 0.f , 0.f, -N.x * 0.1f);
    if (OUT.color.a < 0.01f)
        discard;
    return OUT;
}

