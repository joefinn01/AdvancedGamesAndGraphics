#include "LightHelper.hlsl"

struct VS_INPUT
{
	float3 PosL  : POSITION;
	float3 NormalL : NORMAL;
};

struct VS_OUTPUT
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION;
	float3 NormalW : NORMAL;
};

cbuffer PerFrameCB : register(b0)
{
	float4x4 ViewProjection;
	float4x4 InvTransposeViewProjection;

	Light Lights[MaxLights];

	float4 Ambient;

	float3 EyePosW;
};

cbuffer PerObjectCB : register(b1)
{
	float4x4 world;
}

cbuffer MaterialCB : register(b2)
{
	float4 Diffuse;
	float3 Fresnel;
	float Roughness;
}

VS_OUTPUT VSMain(VS_INPUT input)
{
	VS_OUTPUT result;

	result.PosW = mul(float4(input.PosL, 1.0f), world);
	result.PosH = mul(result.PosW, ViewProjection);

	result.NormalW = mul(float4(input.NormalL, InvTransposeViewProjection));

	return result;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
	input.NormalW = normalize(input.NormalW);

	float3 toEyeW = normalize(EyePosW - input.PosW);

	// Indirect lighting.
	float4 ambient = Ambient * Diffuse;

	Material material = { Diffuse, Fresnel, Roughness };

	float4 directLight = ComputeLighting(Lights, material, input.PosW, input.NormalW, toEyeW);

	float4 litColor = ambient + directLight;

	litColor.a = Diffuse.a;

	return litColor;
}