
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
	float3 Padding;

	int	ImageWidth;
	int	ImageHeight;
	float SigmaD;
	float SigmaR;
};
#ifndef groupthreads
#define groupthreads 32
#endif

#define MINF asfloat(0xff800000)
//------------------------------------------------------------------------------------------------
//Texture2D depthTexture : register(t2);

Texture2D<float> input : register(t0);
Texture2D<float> input4F : register(t1);

RWTexture2D<float> output : register(u0);
RWTexture2D<float> output4F : register(u1);

float GaussR(float sigma, float dist)
{
	return exp(-(dist * dist) / (2.f * sigma * sigma));
}

float GaussD(float sigma, int x, int y)
{
	return exp(-((x * x + y * y) / (2.f * sigma * sigma)));
}

static const float minFilterWeight = 10.0f;

[numthreads(groupthreads, groupthreads, 1)]
void BilateralCompletionCS(int3 dTid : SV_DispatchThreadID)
{
	int kernelRadius = (int)ceil(2.f * SigmaD);
	int kernalSize = 2 * kernelRadius + 1;

	float sum = 0.f;
	float sumWeight = 0.f;
	
	float intCenter = input[dTid.xy];
	output[dTid.xy] = intCenter;

	if (input[dTid.xy] >= 1.f)
	return;

#ifdef WITH_COLOR
	output4F[dTid.xy] = input4F[dTid.xy];
#endif
	
	if (intCenter = MINF)
	{	
		for (int m = dTid.x - kernelRadius; m <= dTid.x + kernelRadius; m++)
		{	
			for (int n = dTid.y - kernelRadius; n <= dTid.y + kernelRadius; n++)
			{	
				if (m >= 0 && n >= 0 && m < ImageWidth && n < ImageHeight)
				{
					uint2 pos = uint2(m, n);
					float intKerPos = input[pos];

					if (intKerPos != MINF)
					{
						float weight = GaussD(SigmaD, m - dTid.x, n - dTid.y);
						sumWeight += weight;
						sum += weight * intKerPos;
					}
				}
			}
		}

		if (sumWeight == 0.f) {return;}
		
		float avgDepth = sum / sumWeight;
		sum = sumWeight = 0.f;
#ifdef WITH_COLOR
		float3 sumColor = 0.f;
#endif
		for (int m1 = dTid.x - kernelRadius; m1 <= dTid.x + kernelRadius; m1++)
		{	
			for (int n1 = dTid.y - kernelRadius; n1 <= dTid.y + kernelRadius; n1++)
			{	
				if (m1 >= 0 && n1 >= 0 && m1 < ImageWidth && n1 < ImageHeight)
				{
					uint2 pos = uint2(m1, n1);
					float intKerPos = input[pos];
					if (intKerPos != MINF)
					{
						float weight = GaussD(SigmaD, m1 - dTid.x, n1 - dTid.y)* GaussR(SigmaR, intKerPos - avgDepth);
						sumWeight += weight;
						sum += weight * intKerPos;
#ifdef WITH_COLOR
						float3 color = input4F[pos].xyz;
						sumColor = weight * color.xyz;
#endif

					}
				}
			}
		}
		
		if (sumWeight > minFilterWeight)
		{
			output[dTid.xy] = sum / sumWeight;
#ifdef WITH_COLOR
			output4F[dTid.xy] = float4(sumColor / sumWeight, 1.f);
#endif
		}
	}
}