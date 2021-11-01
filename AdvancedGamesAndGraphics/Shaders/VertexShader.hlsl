#include "LightHelper.hlsl"

//#define PARALLAX_SHADOW 1

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
    
	float3 viewVectorW = normalize(EyePosW - input.PosW);

#if NORMAL_MAPPING || PARALLAX_MAPPING || PARALLAX_OCCLUSION || PARALLAX_SHADOW
    input.TangentW = normalize(input.TangentW);
    
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
    float3 viewVectorT = mul(viewVectorW, transpose(tbn));
    
    float2 uvOffset = viewVectorT.xy * (1.0f - HeightTex.Sample(SamAnisotropicWrap, input.TexCoords).x) * heightScale;

    float2 uv = input.TexCoords - uvOffset;  
    
    if (uv.x > 1.0 || uv.y > 1.0 || uv.x < 0.0 || uv.y < 0.0)
    {
        discard;
    }
#elif PARALLAX_OCCLUSION || PARALLAX_SHADOW
    float3 viewVectorT = mul(viewVectorW, transpose(tbn));
    
	int minLayers = 5;
	int maxLayers = 15;
	
	int numLayers = lerp(maxLayers, minLayers, abs(dot(float3(0, 0, 1), viewVectorT)));
	
	float2 deltaTex = (viewVectorT.xy * heightScale) / numLayers;
	
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
    float3 normalT = NormalTex.Sample(SamAnisotropicWrap, uv).xyz;
	normalT *= 2.0f;
	normalT -= 1.0f;

	normalT = normalize(normalT);

	float3 bumpedNormalW = mul(normalT, tbn);
    
#if PARALLAX_SHADOW
    float shadowFactor = 0.0f;
    
    float3 lightVecT = mul(normalize(input.PosW - Lights[0].Position), tbn);
    
    if (lightVecT.z < 0.0f && dot(-lightVecT, normalT) > 0.0f)
    {
        numLayers = lerp(maxLayers, minLayers, abs(dot(float3(0, 0, 1), lightVecT)));
        
        currentTex = uv;
        currentDepthMapValue = HeightTex.SampleLevel(SamAnisotropicClamp, currentTex, 0).x;
        currentLayerDepth = currentDepthMapValue;

        layerDepth = (1.0f) / numLayers;
        
        deltaTex = (lightVecT.xy * heightScale) / numLayers;
        
        while (currentDepthMapValue <= currentLayerDepth && currentLayerDepth < 1.0f)
        {
            currentTex -= deltaTex;
            currentDepthMapValue = HeightTex.SampleLevel(SamAnisotropicClamp, currentTex, 0).x;
            currentLayerDepth += layerDepth;
        }

        if (currentDepthMapValue <= currentLayerDepth)
        {
            shadowFactor = 1.0f;
        }
    }
    
#endif
    
	LightingResult result = CalculateLighting(Lights, gMaterial, input.PosW, bumpedNormalW, viewVectorW);
#else
    LightingResult result = CalculateLighting(Lights, gMaterial, input.PosW, input.NormalW, viewVectorW);
#endif

	float4 textureColour = { 1, 1, 1, 1 };

    textureColour = ColorTex.Sample(SamPointWrap, uv);

#if PARALLAX_SHADOW
    float4 litColour = saturate(textureColour * (result.Ambient + (result.Diffuse * shadowFactor)) + result.Specular * shadowFactor);
#else
	float4 litColour = saturate(textureColour * (result.Ambient + result.Diffuse) + result.Specular);
#endif

	litColour.a = gMaterial.Diffuse.a;

	return litColour;
}