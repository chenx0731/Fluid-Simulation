#include "Constants.hlsl"
//-----------------------------------------------------------------------------------------------

struct VSInput
{
    //uint vidx : SV_VERTEXID;
    uint instanceID : SV_InstanceID;
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
    float2 texCoord : TEXCOORD0;
    float size : TEXCOORD1;
};

struct GSOutput
{
    float4 position : SV_Position;
    float4 viewPos : POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD0;
    float size : TEXCOORD1;
};

struct PSOutput
{
    float4 color : SV_TARGET0;
};

cbuffer LightConstants : register(b1)
{
    float3 LightDirt;
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

cbuffer BlurConstants : register(b6)
{
	float FilterRadius;
	float2 BlurDir;
	float BlurScale;

	float BlurDepthFalloff;
	float3 Padding;

	uint    ImageWidth;
	uint	ImageHeight;
	float SigmaD;
	float SigmaR;
};

Texture2D<float2> intersections : register(t0);

#define MAXF 9999.f
//------------------------------------------------------------------------------------------------
VSOutput VertexMain(uint ID : SV_VertexID)
{
    VSOutput OUT = (VSOutput)0;
    uint y = ID / ImageWidth;
    uint x = ID - y * ImageWidth;
    uint2 index = uint2(x,y);
    float2 intersection = intersections[index].xy;
    //if (intersection.x > MAXF)
        //discard;
    OUT.position = float4(intersection, -0.99f, 1.f);
    //OUT.size = 0.2f / distanceToCamera;
    OUT.size = 0.05f ;

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

    float3 up = float3(1.f, 0.f, 0.f);
//CameraUpDir.xyz;
    //float3(0.0f, 0.0f, 1.0f);
    //float3 look = EyeSpacePos - In[0].position.xyz;
    //look = normalize(look);
    //float3 right = cross(up, look);
    float3 right = float3(0.f, -1.f, 0.f);
//-CameraLeftDir.xyz;
//float3(0.f, 0.f, 0.f); 
    //look.z = 0.f; // project to xy-plane
/*
    if (length(look) != 0.f)    
        look = normalize(look);
    else {
        right = 
    }
    float3 right = cross(up, look);
    if (length(look) == 0.f)
    	else {
			i = -forward;
			j = -left;
			k = up;
		}
		if (fabs(i.z) < 1) {
			j = CrossProduct3D(z, i);
			j.Normalize();
			k = CrossProduct3D(i, j);
			k.Normalize();
		}
		else {
			k = CrossProduct3D(i, y);
			k.Normalize();
			j = CrossProduct3D(k, i);
			j.Normalize();
		}
    */
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
        gsout.viewPos = viewPosition;
        gsout.color = float4(0.f, 1.f, 1.f, 0.7f);

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
    
    float3 N;
    N.yz = input.texCoord * 2.0 - 1.0;
    N.y = -N.y;
    float r2 = dot(N.yz, N.yz);
    if (r2 > 1.0)
        discard;
    N.x = -sqrt(1.0 - r2);
    
    OUT.color = float4(1.f, 1.f , 1.f, 0.015f * -N.x);
    //if (OUT.color.a < 0.01f)
        //discard;
    return OUT;

}
