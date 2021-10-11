
#define MAX_LIGHTS 16

#define SPOT 0
#define POINT 1
#define DIRECTIONAL 2

struct Material
{
	float4 Diffuse;
	float3 Fresnel;
	float Roughness;
};

struct Light
{
	float3 Direction;
	float FallOffStart;
	float3 Color;
	float FallOffEnd;
	float3 Position;
	float SpotLightPower;
	int InUse;
	int Type;
	float2 pad;
};

float3 Schlick(float3 fFresnel, float3 normal, float3 lightDir)
{
	float fValue = 1.0f - saturate(dot(normal, lightDir));

	return fFresnel + (1.0f - fFresnel) * fValue * fValue * fValue * fValue * fValue;
}

float CalculateAttenuation(float fDist, float fStart, float fEnd)
{
	return saturate((fEnd - fDist) / (fEnd - fStart));
}

float3 BlinnPhong(float3 color, float3 lightDir, float3 normal, float3 toEye, Material material)
{
	float fValue = (1.0f - material.Roughness) * 256.0f;

	float3 h = normalize(toEye + lightDir);

	//Roughness factor * fresnel factor
	float3 specular = ((fValue + 8.0f) * pow(max(dot(h, normal), 0.0f), fValue) / 8.0f) * Schlick(material.Fresnel, h, lightDir);

	//Remap to between 0 and 1
	specular = specular / (specular + 1.0f);

	return (material.Diffuse.rgb + specular) * color;
}

float3 CalculatePoint(Light light, Material material, float3 position, float3 normal, float3 toEye)
{
	float3 lightDir = light.Position - position;

	float fDistance = length(lightDir);

	if (fDistance > light.FallOffEnd)
	{
		return 0.0f;
	}

	lightDir /= fDistance;

	float3 color = light.Color * max(dot(lightDir, normal), 0.0f) * CalculateAttenuation(fDistance, light.FallOffStart, light.FallOffEnd);

	return BlinnPhong(color, lightDir, normal, toEye, material);
}

float3 CalculateSpot(Light light, Material material, float3 position, float3 normal, float3 toEye)
{
	float3 lightDir = light.Position - position;

	float fDistance = length(lightDir);

	if (fDistance > light.FallOffEnd)
	{
		return 0.0f;
	}

	lightDir /= fDistance;

	// color * lambert * attenuation * spotlight factor
	float3 color = light.Color * max(dot(lightDir, normal), 0.0f) * CalculateAttenuation(fDistance, light.FallOffStart, light.FallOffEnd) * pow(max(dot(-lightDir, light.Direction), 0.0f), light.SpotLightPower);

	return BlinnPhong(color, lightDir, normal, toEye, material);
}

float3 CalculateDirectional(Light light, Material material, float3 normal, float3 toEye)
{
	float3 lightDir = -light.Direction;

	float3 color = light.Color * max(dot(lightDir, normal), 0.0f);

	return BlinnPhong(color, lightDir, normal, toEye, material);
}

float4 CalculateLighting(Light lights[MAX_LIGHTS], Material material, float3 position, float3 normal, float3 toEye)
{
	float3 result = 0.0f;

	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		if (lights[i].InUse == 0)
		{
			continue;
		}

		if (lights[i].Type == SPOT)
		{
			result += CalculateSpot(lights[i], material, position, normal, toEye);
		}
		else if (lights[i].Type == POINT)
		{
			result += CalculatePoint(lights[i], material, position, normal, toEye);
		}
		else
		{
			result += CalculateDirectional(lights[i], material, normal, toEye);
		}
	}

	return float4(result, 1.0f);
}