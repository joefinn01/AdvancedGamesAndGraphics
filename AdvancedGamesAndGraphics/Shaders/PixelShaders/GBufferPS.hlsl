#include "../Common/LightHelper.hlsl"

#include "../Common/Samplers.hlsl"

struct PS_INPUT
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexCoords : TEXCOORD;
    float3 TangentW : TANGENT;
};

struct PS_OUTPUT
{
    float4 Albedo : SV_TARGET0;
    float4 NormalW : SV_TARGET1;
    float4 TangentW : SV_TARGET2;
    float4 Diffuse : SV_TARGET3;
    float4 Specular : SV_TARGET4;
    float4 Ambient : SV_TARGET5;
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

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    
    output.Albedo = ColorTex.Sample(SamPointWrap, input.TexCoords);
    output.NormalW = float4(normalize(input.NormalW), 0);
    output.TangentW = float4(normalize(input.TangentW), 0);
    output.Diffuse = gMaterial.Diffuse;
    output.Specular = gMaterial.Specular;
    output.Ambient = gMaterial.Ambient;
    
    return output;
}