#include "LightHelper.hlsl"

struct VS_INPUT
{
	float3 PosL  : POSITION;
	float3 NormalL : NORMAL;
	float2 TexCoords : TEXCOORD;
	float3 Tangent : TANGENT;
};

struct VS_OUTPUT
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float2 TexCoords : TEXCOORD;
	float3 TangentW : TANGENT;
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
Texture2D NormalTex : register(t4);
Texture2D HeightTex : register(t5);

SamplerState SamPointWrap        : register(s0);
SamplerState SamPointClamp       : register(s1);
SamplerState SamLinearWrap       : register(s2);
SamplerState SamLinearClamp      : register(s3);
SamplerState SamAnisotropicWrap  : register(s4);
SamplerState SamAnisotropicClamp : register(s5);

VS_OUTPUT VSMain(VS_INPUT input)
{
	VS_OUTPUT result;

	result.PosW = mul(float4(input.PosL, 1.0f), World).xyz;
	result.PosH = mul(float4(result.PosW, 1.0f), ViewProjection);
	 
	result.NormalW = normalize(mul(float4(input.NormalL, 0), TransposeInvWorld).xyz);
	result.TangentW = normalize(mul(float4(input.Tangent, 0), TransposeInvWorld).xyz);

	result.TexCoords = input.TexCoords;

	return result;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
	input.NormalW = normalize(input.NormalW);
	input.TangentW = normalize(input.TangentW);

	//Calculate TBN matrix
	float3 n = input.NormalW;
	float3 t = normalize(input.TangentW - dot(input.TangentW, n) * n);	//Ensures the vectors are orthogonal
	float3 b = cross(n, t);

	float3x3 tbn = float3x3(t, b, n);

	float3 viewVectorW = normalize(EyePosW - input.PosW);

	float3 viewVectorT = mul(viewVectorW, transpose(tbn));
	//float3 viewVectorT = normalize(mul(EyePosW, transpose(tbn)) - mul(input.PosW, transpose(tbn)));

	float height = HeightTex.Sample(SamAnisotropicWrap, input.TexCoords).x;
	float heightScale = 0.1f;

	//Divide by Z axis to remove issues at steeper angles.
	//float2 uvOffset = (viewVectorT.xy / viewVectorT.z) * (1.0f - height) * heightScale;
	float2 uvOffset = viewVectorT.xy * (1.0f - height) * heightScale;

	float2 uv = input.TexCoords - uvOffset;
	//float2 uv = input.TexCoords;

	if (uv.x > 1.0 || uv.y > 1.0 || uv.x < 0.0 || uv.y < 0.0)
	{
		discard;
	}

	float3 normalT = NormalTex.Sample(SamAnisotropicWrap, uv).xyz;
	normalT *= 2.0f;
	normalT -= 1.0f;

	normalT = normalize(normalT);

	float3 bumpedNormalW = mul(normalT, tbn);

	//LightingResult result = CalculateLighting(Lights, gMaterial, input.PosW, input.NormalW, normalize(viewVectorW));	//Normalize as interpolation can cause vector not to be normal
	LightingResult result = CalculateLighting(Lights, gMaterial, input.PosW, bumpedNormalW, viewVectorW);

	float4 textureColour = { 1, 1, 1, 1 };

	textureColour = ColorTex.Sample(SamPointWrap, uv);
	//textureColour = ColorTex.Sample(SamPointWrap, input.TexCoords);

	float4 litColour = saturate(textureColour * (result.Ambient + result.Diffuse) + result.Specular);

	litColour.a = gMaterial.Diffuse.a;

	return litColour;
}