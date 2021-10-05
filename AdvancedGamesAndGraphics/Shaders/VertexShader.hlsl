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

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT result;

	result.PosH = mul(float4(input.PosL.xyz, 1.0f), viewProjection);
	//result.PosH = float4(input.PosL.xyz, 1.0f);
	result.Color = input.Color;

	return result;
}