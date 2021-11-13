struct PS_INPUT
{
	float4 PosH : SV_POSITION;
	float2 TexCoords : TEXCOORD;
};

#include "../Common/LightHelper.hlsl"

#include "../Common/Samplers.hlsl"

cbuffer PerFrameCB : register(b0)
{
	float4x4 InvTransposeViewProjection;

	float3 EyePosW;
	int ScreenWidth;
	
    int ScreenHeight;
    float3 pad;
};

cbuffer LigtPassCB : register(b1)
{
    Light light;
};

Texture2D AlbedoTex : register(t0);
Texture2D NormalTex : register(t1);
Texture2D TangentTex : register(t2);
Texture2D DiffuseTex : register(t3);
Texture2D SpecularTex : register(t4);
Texture2D AmbientTex : register(t5);
Texture2D DepthTex : register(t6);

float4 main(PS_INPUT input) : SV_TARGET
{
	LightingResult result = InitLight();
		
	float3 albedo = AlbedoTex.Sample(SamLinearClamp, input.TexCoords).xyz;
    float3 normalW = normalize((NormalTex.Sample(SamLinearClamp, input.TexCoords).xyz * 2.0f) - float3(1.0f, 1.0f, 1.0f));
	float4 ambient = AmbientTex.Sample(SamLinearClamp, input.TexCoords);
	float4 diffuse = DiffuseTex.Sample(SamLinearClamp, input.TexCoords);
	float4 specular = SpecularTex.Sample(SamLinearClamp, input.TexCoords);
    float depth = DepthTex.Sample(SamLinearClamp, input.TexCoords);

    float4 posW = float4(input.TexCoords.x * 2.0f - 1.0f, input.TexCoords.y * 2.0f - 1.0f, depth, 1.0f);
    posW.y *= -1.0f;
	
    posW = mul(posW, InvTransposeViewProjection); //Calculate world pos by transforming from clip space
    posW = posW / posW.w;
	
    float3 viewVectorW = normalize(EyePosW - posW.xyz);
	
	switch (light.LightType)
	{
		case DIRECTIONAL:
			result = CalculateDirectional(light, InitMaterial(ambient, diffuse, specular), normalW, viewVectorW);
			break;

		case POINT:
			result = CalculatePoint(light, InitMaterial(ambient, diffuse, specular), posW.xyz, normalW, viewVectorW);
			break;

		case SPOT:
            result = CalculateSpot(light, InitMaterial(ambient, diffuse, specular), posW.xyz, normalW, viewVectorW);
			break;
	}
	
    float4 litColour = float4(saturate(albedo * (result.Ambient.xyz + result.Diffuse.xyz) + result.Specular.xyz), 1.0f);
	
	litColour.a = diffuse.a;
	
	return litColour;
}