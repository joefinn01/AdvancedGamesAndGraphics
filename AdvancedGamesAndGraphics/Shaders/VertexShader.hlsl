#include "LightHelper.hlsl"

struct VS_INPUT
{
	float3 PosL  : POSITION;
	float3 NormalL : NORMAL;
	float2 TexCoords : TEXCOORD;
};

struct VS_OUTPUT
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION;
	float4 NormalW : NORMAL;
	float2 TexCoords : TEXCOORD;
};

cbuffer PerFrameCB : register(b0)
{
	float4x4 ViewProjection;
	float4x4 InvTransposeViewProjection;

	Light Lights[MAX_LIGHTS];

	float4 Ambient;

	float3 EyePosW;
	float pad;
};

cbuffer PerObjectCB : register(b1)
{
	float4x4 World;
	float4x4 TransposeInvWorld;
}

cbuffer MaterialCB : register(b2)
{
	Material gMaterial;
}

Texture2D ColorTex : register(t3);

SamplerState SamplePointWrap : register(s0);

VS_OUTPUT VSMain(VS_INPUT input)
{
	VS_OUTPUT result;

	result.PosW = mul(float4(input.PosL, 1.0f), World).xyz;
	result.PosH = mul(float4(result.PosW, 1.0f), ViewProjection);

	result.NormalW = normalize(mul(float4(input.NormalL, 1.0f), TransposeInvWorld));

	result.TexCoords = input.TexCoords;

	return result;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
	float3 viewVector = EyePosW - input.PosW;

	LightingResult result = CalculateLighting(Lights, gMaterial, input.PosW, normalize(input.NormalW).xyz, normalize(viewVector));	//Normalize as interpolation can cause vector not to be normal

	float4 textureColour = { 1, 1, 1, 1 };

	textureColour = ColorTex.Sample(SamplePointWrap, input.TexCoords);

	float4 litColour = textureColour * (result.Ambient + result.Diffuse) + result.Specular;

	litColour.a = gMaterial.Diffuse.a;

	return litColour;
}