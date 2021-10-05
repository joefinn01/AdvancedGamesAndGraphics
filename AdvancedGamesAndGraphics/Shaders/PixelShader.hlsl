Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

struct VS_OUTPUT
{
	float4 PosH  : SV_POSITION;
	float4 Color : COLOR;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
	return input.Color;
}