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
	float4 NormalW : NORMAL;
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

	result.PosW = mul(float4(input.PosL, 1.0f), World).xyz;
	result.PosH = mul(float4(result.PosW, 1.0f), ViewProjection);

	result.NormalW = mul(float4(input.NormalL, 1.0f), InvTransposeViewProjection);

	return result;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
	input.NormalW = normalize(input.NormalW);

	float3 toEyeW = normalize(EyePosW - input.PosW);

	// Indirect lighting.
	float4 ambient = Ambient * Diffuse;

	//Material material = { Diffuse, Fresnel, Roughness };

	Light light;
	light.Position = float3(0, 0, 0);
	light.FallOffStart = 20.0f;
	light.FallOffEnd = 60.0f;
	light.Color = float3(0.0f, 1.0f, 0.0f);

	Material material = {float4(0.6f, 0.0f, 0.0f, 1.0f), float3(0.2f, 0.2f, 0.2f), 0.4f};

	//float4 directLight = CalculateLighting(Lights, material, input.PosW, input.NormalW.xyz, toEyeW);
	float4 directLight = float4(CalculatePoint(light, material, input.PosW, input.NormalW.xyz, toEyeW), 1.0f);

	float4 litColor = Ambient * 0.1f + directLight;

	litColor.a = Diffuse.a;

	return directLight;
}