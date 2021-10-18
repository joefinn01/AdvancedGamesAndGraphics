#include "LightHelper.hlsl"

struct VS_INPUT
{
	float3 PosL  : POSITION;
	float3 NormalL : NORMAL;
	float2 TexCoords : TEXCOORD;
	float3 Tangent : TANGENT;
	float3 Bitangent : BITANGENT;
};

struct VS_OUTPUT
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION;
	float4 NormalW : NORMAL;
	float2 TexCoords : TEXCOORD;
	float3 EyeVecTS : TEST;
	float3 LightVecTS : TEST2;
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
Texture2D BumpTex : register(t4);

SamplerState SamplePointWrap : register(s0);

VS_OUTPUT VSMain(VS_INPUT input)
{
	VS_OUTPUT result;

	result.PosW = mul(float4(input.PosL, 1.0f), World).xyz;
	result.PosH = mul(float4(result.PosW, 1.0f), ViewProjection);

	result.NormalW = normalize(mul(float4(input.NormalL, 1.0f), TransposeInvWorld));

	result.TexCoords = input.TexCoords;

	// Build TBN matrix
	float3 T = normalize(mul(input.Tangent, TransposeInvWorld));
	float3 B = normalize(mul(input.Bitangent, TransposeInvWorld));
	float3 N = result.NormalW;
	float3x3 TBN = float3x3(T, B, N);
	float3x3 TBN_inv = transpose(TBN);

	result.EyeVecTS = normalize(mul(EyePosW - result.PosW, TBN_inv));
	result.EyeVecTS = normalize(mul(EyePosW - Lights[0].Position, TBN_inv));

	return result;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
	float3 viewVector = EyePosW - input.PosW;

	float4 bumpMap = BumpTex.Sample(SamplePointWrap, input.TexCoords);
	bumpMap = (bumpMap * 2.0f) - 1.0f;
	bumpMap = float4(normalize(bumpMap.xyz), 1);


	LightingResult result = CalculateLighting(Lights, gMaterial, input.PosW, bumpMap.xyz, normalize(input.EyeVecTS));	//Normalize as interpolation can cause vector not to be normal

	float4 textureColour = { 1, 1, 1, 1 };

	textureColour = ColorTex.Sample(SamplePointWrap, input.TexCoords);

	float4 litColour = textureColour * (result.Ambient + result.Diffuse) + result.Specular;

	litColour.a = gMaterial.Diffuse.a;

	return litColour;
}