struct VS_INPUT
{
	float3 PosL  : POSITION;
	float4 Color : COLOR;
	float3 Normal : NORMAL;
};

struct VS_OUTPUT
{
	float4 PosH  : SV_POSITION;
	float4 Color : COLOR;
};

cbuffer PerFrameCB : register(b0)
{
	float4x4 viewProjection;
};

cbuffer PerObjectCB : register(b1)
{
	float4x4 world;
}

cbuffer MaterialCB : register(b2)
{
	float4 diffuse;
	float3 fresnel;
	float roughness;
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT result;

	result.PosH = mul(mul(float4(input.PosL, 1.0f), world), viewProjection);
	result.Color = input.Color;

	return result;
}