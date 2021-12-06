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
    float4 Shadow : SV_TARGET6;
};

cbuffer PerFrameCB : register(b0)
{
    float4x4 ViewProjection;

    Light light;

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

Texture2D ColorTex : register(t0);
Texture2D NormalTex : register(t1);
Texture2D HeightTex : register(t2);

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    
#if NORMAL_MAPPING || PARALLAX_MAPPING || PARALLAX_OCCLUSION || PARALLAX_SHADOW
    input.TangentW = normalize(input.TangentW);
    input.NormalW = normalize(input.NormalW);

    //Calculate TBN matrix
    float3 n = input.NormalW;
    float3 t = normalize(input.TangentW - dot(input.TangentW, n) * n);	//Ensures the vectors are orthogonal
    float3 b = cross(n, t);

    float3x3 tbn = float3x3(t, b, n);

    float heightScale = 0.1f;
#endif

#if NORMAL_MAPPING
    float2 uv = input.TexCoords;
#elif PARALLAX_MAPPING
    /***********************************************
    MARKING SCHEME: parallax Mapping
    DESCRIPTION: Standard Parallax
    ***********************************************/


    float3 viewVectorW = normalize(EyePosW - input.PosW);
    float3 viewVectorT = mul(viewVectorW, transpose(tbn));

    float2 uvOffset = viewVectorT.xy * (1.0f - HeightTex.Sample(SamAnisotropicWrap, input.TexCoords).x) * heightScale;

    float2 uv = input.TexCoords - uvOffset;

#elif PARALLAX_OCCLUSION || PARALLAX_SHADOW
    /***********************************************
    MARKING SCHEME: parallax occlusion
    DESCRIPTION: Parallax Occlusion
    ***********************************************/


    float3 viewVectorW = normalize(EyePosW - input.PosW);
    float3 viewVectorT = mul(viewVectorW, transpose(tbn));

    int minLayers = 50;
    int maxLayers = 100;

    int numLayers = lerp(maxLayers, minLayers, abs(dot(float3(0, 0, 1), viewVectorT)));

    float2 deltaTex = (viewVectorT.xy * heightScale) / (numLayers);

    float2 currentTex = input.TexCoords;

    float currentLayerDepth = 0;
    float height = HeightTex.Sample(SamAnisotropicClamp, currentTex).x;
    float currentDepthMapValue = height;

    float layerDepth = (1.0f) / numLayers;

    while (currentLayerDepth < currentDepthMapValue)
    {
        currentTex += deltaTex;

        currentDepthMapValue = HeightTex.SampleLevel(SamAnisotropicClamp, currentTex, 0).x;

        currentLayerDepth += layerDepth;
    }

    float2 prevTex = currentTex - deltaTex;

    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = HeightTex.Sample(SamAnisotropicClamp, prevTex).x - currentLayerDepth + layerDepth;

    float weight = afterDepth / (afterDepth - beforeDepth);
    float2 uv = prevTex * weight + currentTex * (1.0 - weight);
#else 
    float2 uv = input.TexCoords;
#endif

#if NORMAL_MAPPING || PARALLAX_MAPPING || PARALLAX_OCCLUSION || PARALLAX_SHADOW
    
    /***********************************************
    MARKING SCHEME: Normal Mapping
    DESCRIPTION: Map sampling, normal value decompression, transformation to tangent space
    ***********************************************/


    float3 normalT = NormalTex.Sample(SamAnisotropicWrap, uv).xyz;
    normalT *= 2.0f;
    normalT -= 1.0f;

    normalT = normalize(normalT);

    float3 bumpedNormalW = mul(normalT, tbn);

#if PARALLAX_SHADOW

    /***********************************************
    MARKING SCHEME: Self-shadowing parallax mapping
    DESCRIPTION: Self-shadowing parallax mapping
    ***********************************************/


    output.Shadow = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float3 lightVecT = mul(normalize(input.PosW - light.Position), transpose(tbn));

    if (lightVecT.z < 0.0f && dot(-lightVecT, normalT) > 0.0f)
    {
        numLayers = lerp(maxLayers, minLayers, abs(dot(float3(0, 0, 1), lightVecT)));

        currentTex = uv;
        currentDepthMapValue = HeightTex.SampleLevel(SamAnisotropicClamp, currentTex, 0).x;
        currentLayerDepth = currentDepthMapValue;

        layerDepth = (1.0f) / numLayers;

        deltaTex = (lightVecT.xy * heightScale) / (numLayers);

        while (currentDepthMapValue <= currentLayerDepth && currentLayerDepth < 1.0f)
        {
            currentTex -= deltaTex;
            currentDepthMapValue = HeightTex.SampleLevel(SamAnisotropicClamp, currentTex, 0).x;
            currentLayerDepth += layerDepth;
        }

        if (currentDepthMapValue <= currentLayerDepth)
        {
            output.Shadow = float4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
#endif
#endif

    output.Albedo = ColorTex.Sample(SamPointWrap, uv);

#if NORMAL_MAPPING || PARALLAX_MAPPING || PARALLAX_OCCLUSION || PARALLAX_SHADOW
    output.NormalW = float4((bumpedNormalW + float3(1, 1, 1)) * 0.5f, 0); //Need to remap vector values to be between 0 and 1
#else
    output.NormalW = float4((normalize(input.NormalW) + float3(1, 1, 1)) * 0.5f, 0); //Need to remap vector values to be between 0 and 1
#endif

    output.TangentW = float4((normalize(input.TangentW) + float3(1, 1, 1)) * 0.5f, 0);
    output.Diffuse = gMaterial.Diffuse;
    output.Specular = gMaterial.Specular;
    output.Ambient = gMaterial.Ambient;
    
    return output;
}