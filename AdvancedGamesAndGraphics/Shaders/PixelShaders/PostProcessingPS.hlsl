struct PS_INPUT
{
	float4 PosH : SV_POSITION;
	float2 TexCoords : TEXCOORD;
};

Texture2D CurrentTex : register(t0);

float4 main(PS_INPUT input) : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}