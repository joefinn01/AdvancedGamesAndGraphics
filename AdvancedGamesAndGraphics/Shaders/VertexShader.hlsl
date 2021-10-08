struct VS_INPUT
{
	float4 PosL  : POSITION;
	float4 Color : COLOR;
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

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT result;

	result.PosH = mul(mul(float4(input.PosL.xyz, 1.0f), world), viewProjection);
	result.Color = input.Color;

	return result;
}