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
};

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
    float4 CameraUpDirt;
    /*
	float2 DepthTextureSize;
    float Padding2;
    float Padding3;
    float4 WaterColor;*/
};
cbuffer BlurConstants : register(b6)
{
	float FilterRadius;
	float2 BlurDir;
	float BlurScale;

	float BlurDepthFalloff;
	float3 Padding4; 
};

//------------------------------------------------------------------------------------------------
Texture2D depthTexture : register(t2);

//------------------------------------------------------------------------------------------------
SamplerState depthSampler : register(s0);

//------------------------------------------------------------------------------------------------
VSOutput VertexMain(VSInput input)
{
	VSOutput v2p;
    v2p.position = float4(FS_POS[input.vidx], 1.f);
	v2p.color = 1.f.xxxx;
	v2p.uv = FS_UVS[input.vidx];
	return v2p;
}

//------------------------------------------------------------------------------------------------
PSOutput PixelMain(VSOutput input)
{ 
	PSOutput OUT;
	float depth = depthTexture.Sample(depthSampler, input.uv).x;

	float sum = 0;
	float wsum = 0;
/*
	[unroll(60)]
	for (float x = -FilterRadius; x <= FilterRadius; x += 1.0)	 {

		float sample = depthTexture.Sample(depthSampler, (input.uv + x * BlurDir)).x;
		
		if (sample >= 1.f) continue;		

		float r = x * BlurScale;
		float w = exp(-r * r);

		float r2 = (sample - depth) * BlurDepthFalloff;
//65.f;
		float g = exp(-r2 * r2);

		sum += sample * w * g;
		wsum += w * g;
	}
	
	if (wsum > 0)
		sum /= wsum;
	*/
	
	for (float x = -FilterRadius; x <= FilterRadius; x += 1.0)	 {
		[unroll(15)]
		for (float y = -FilterRadius; y <= FilterRadius; y += 1.0) {
			
			float sample = depthTexture.Sample(depthSampler, (input.uv + float2(x, y) * BlurDir)).r;
			if (sample >= 1.f) continue;		

			float r = length(float2(x, y)) * BlurScale;
			float w = exp(-r * r);

			float r2 = (sample - depth) * BlurDepthFalloff;
	//65.f;
			float g = exp(-r2 * r2);

			sum += sample * w * g;
			wsum += w * g;
		}
	}
	

	if (wsum > 0)
		sum /= wsum;
	
	OUT.color.x = sum;
	
	if (depth >= 1.0 || depth == 0.f)
	{
		OUT.color.x = depth;
	}
	
	OUT.color.yzw = depthTexture.Sample(depthSampler, input.uv).yzw;

	return OUT;
}