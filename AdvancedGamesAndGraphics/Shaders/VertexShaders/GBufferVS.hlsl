
#include "../Common/LightHelper.hlsl"

struct VS_INPUT
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexCoords : TEXCOORD;
    float3 Tangent : TANGENT;
};

struct VS_OUTPUT
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexCoords : TEXCOORD;
    float3 TangentW : TANGENT;
};

cbuffer PerFrameCB : register(b0)
{
    float4x4 ViewProjection;
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

Texture2D ColorTex : register(t0);
Texture2D NormalTex : register(t1);
Texture2D HeightTex : register(t2);

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT result;

    result.PosW = mul(float4(input.PosL, 1.0f), World).xyz;
    result.PosH = mul(float4(result.PosW, 1.0f), ViewProjection);
	 
    result.NormalW = normalize(mul(float4(input.NormalL, 0), TransposeInvWorld).xyz);
    result.TangentW = normalize(mul(float4(input.Tangent, 0), TransposeInvWorld).xyz);

    result.TexCoords = input.TexCoords;

    return result;
}