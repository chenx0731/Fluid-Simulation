//------------------------------------------------------------------------------------------------
struct vs_input_t
{
	float3 localPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float3 localTangent : TANGENT;
	float3 localBinormal : BINORMAL;
	float3 localNormal : NORMAL;
};

//------------------------------------------------------------------------------------------------
struct v2p_t
{
	float4 clipPosition : SV_Position;
	float4 worldPosition : POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
	float4 worldTangent : TANGENT;
	float4 worldBinormal : BINORMAL;
	float4 worldNormal : NORMAL;
};

//------------------------------------------------------------------------------------------------
/*
cbuffer LightConstants : register(b1)
{
	float3 LightPos;
	float SunIntensity;

	float4 AmbientIntensity;

	float3 WorldEyePosition;
	float SpecularIntensity;

	float SpecularPower;
	float Padding;
	float Padding1;
	float Padding2;
};*/
cbuffer LightConstants : register(b1)
{
    float3 WorldEyePosition;
    float  SpecularPower;

    float3 LightPos;
    float  FresPower;

    float  FresScale;
    float  FresBias;
    float  SunIntensity;
    float  SpecularIntensity;

    float4x4 InvSkyboxModelMatrix;

	float4 AmbientIntensity;
}

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

//------------------------------------------------------------------------------------------------
Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);

//------------------------------------------------------------------------------------------------
SamplerState samplerState : register(s0);

//------------------------------------------------------------------------------------------------
v2p_t VertexMain(vs_input_t input)
{
	float4 worldPosition = mul(ModelMatrix, float4(input.localPosition, 1));
	float4 viewPosition = mul(ViewMatrix, worldPosition);
	float4 clipPosition = mul(ProjectionMatrix, viewPosition);

	v2p_t v2p;
	v2p.clipPosition = clipPosition;
	v2p.worldPosition = worldPosition;
	v2p.color = input.color;
	v2p.uv = input.uv;
	v2p.worldTangent = mul(ModelMatrix, float4(input.localTangent, 0));
	v2p.worldBinormal = mul(ModelMatrix, float4(input.localBinormal, 0));
	v2p.worldNormal = mul(ModelMatrix, float4(input.localNormal, 0));
	return v2p;
}

//------------------------------------------------------------------------------------------------
float4 PixelMain(v2p_t input) : SV_Target0
{
	float4 textureColor = diffuseTexture.Sample(samplerState, input.uv);

	float3 worldNormal;

	float3x3 TBNMatrix = float3x3(normalize(input.worldTangent.xyz), normalize(input.worldBinormal.xyz), normalize(input.worldNormal.xyz));
	float3 tangentNormal = 2.0f * normalTexture.Sample(samplerState, input.uv).rgb - 1.0f;
	worldNormal = mul(tangentNormal, TBNMatrix);
	
	float3 lightDirection = input.worldPosition - LightPos;
	
	float invDistanceFromLight = length(lightDirection);
	if (invDistanceFromLight != 0.f)
		invDistanceFromLight = 1.f / invDistanceFromLight;
	lightDirection = normalize(lightDirection);

	float specularIntensity = 0.0f;
	float specularPower = 0.0f;
	specularIntensity = SpecularIntensity;
	specularPower = SpecularPower;

	float3 worldViewDirection = normalize(WorldEyePosition - input.worldPosition.xyz);
	float3 worldHalfVector = normalize(-lightDirection + worldViewDirection);
	float ndotH = saturate(dot(worldNormal, worldHalfVector));
	float specular = pow(ndotH, specularPower) * specularIntensity * invDistanceFromLight;

	float4 ambient = AmbientIntensity;
	float directional = SunIntensity * saturate(dot(normalize(worldNormal), -lightDirection)) * invDistanceFromLight;
	float4 lightColor = saturate(float4((ambient.x + directional + specular).xxx, 1));
	float4 vertexColor = input.color;
	float4 modelColor = ModelColor;
	float4 color = lightColor * textureColor * vertexColor * modelColor;
	clip(color.a - 0.01f);
	return color;
}
