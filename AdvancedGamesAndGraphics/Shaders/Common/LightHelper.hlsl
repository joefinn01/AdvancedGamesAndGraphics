
#define MAX_LIGHTS 16

#define SPOT 0
#define POINT 1
#define DIRECTIONAL 2

struct Material
{
    float4 Ambient;
    float4 Diffuse; //4th float is the alpha
    float4 Specular; //4th float is the specular power
};

struct LightingResult
{
	float4 Ambient;
	float4 Diffuse;
	float4 Specular;
};

struct Light
{
    float3 Position;
    float Range;

    float4 Direction;

    float3 Ambient;
    int LightType;

    float3 Diffuse;
    int Enabled;

    float4 Specular;

    float3 Attenuation;
    float SpotLightAngle;
};

LightingResult InitLight()
{
	LightingResult result;

	result.Ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	result.Diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	result.Specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

	return result;
}

LightingResult CalculatePoint(Light light, Material material, float3 position, float3 normal, float3 viewVector)
{
	LightingResult result = InitLight();

	float3 lightVec = light.Position - position;

	float fDistance = length(lightVec);

	if (fDistance > light.Range)
	{
		return result;
	}

	lightVec /= fDistance;

	result.Ambient = material.Ambient * float4(light.Ambient, 1.0f);

	float diffuseIntensity = min(dot(lightVec, normal), 1);

	if (diffuseIntensity > 0.0f)
	{
		float test = dot(-lightVec, normal);

		float3 reflected = reflect(-lightVec, normal);
		float specularIntensity = min(pow(max(dot(reflected, viewVector), 0.0f), material.Specular.w), 1.0f);

		result.Diffuse = diffuseIntensity * material.Diffuse * float4(light.Diffuse, 1.0f);
		result.Specular = specularIntensity * material.Specular * light.Specular;
	}

	float attenuation = min(1.0f / dot(light.Attenuation[0], float3(1.0f, light.Attenuation[1] * fDistance, light.Attenuation[2] * fDistance * fDistance)), 1.0f);

	result.Diffuse *= attenuation;
	result.Specular *= attenuation;

	return result;
}

LightingResult CalculateSpot(Light light, Material material, float3 position, float3 normal, float3 viewVector)
{
	LightingResult result = InitLight();

	float3 lightVec = light.Position - position;

	float fDistance = length(lightVec);

	if (fDistance > light.Range)
	{
		return result;
	}

	lightVec /= fDistance;

	result.Ambient = material.Ambient * float4(light.Ambient, 1.0f);

	float diffuseIntensity = min(dot(lightVec, normal), 1.0f);

	if (diffuseIntensity > 0.0f)
	{
		float3 reflected = reflect(-lightVec, normal);
		float specularIntensity = min(pow(max(dot(reflected, viewVector), 0.0f), material.Specular.w), 1.0f);

		result.Diffuse = diffuseIntensity * material.Diffuse * float4(light.Diffuse, 1.0f);
		result.Specular = specularIntensity * material.Specular * light.Specular;
	}

	float spotLightIntensity = pow(max(dot(-lightVec, light.Direction.xyz), 0.0f), light.SpotLightAngle);

	float attenuation = min(1.0f / dot(light.Attenuation[0], float3(1.0f, light.Attenuation[1] * fDistance, light.Attenuation[2] * fDistance * fDistance)), 1.0f);

	result.Ambient *= spotLightIntensity;
	result.Diffuse *= attenuation * spotLightIntensity;
	result.Specular *= attenuation * spotLightIntensity;

	return result;
}

LightingResult CalculateDirectional(Light light, Material material, float3 normal, float3 viewVector)
{
	LightingResult result = InitLight();

	float3 lightVec = -light.Direction.xyz;

	result.Ambient = material.Ambient * float4(light.Ambient, 1.0f);

	float diffuseIntensity = min(dot(lightVec, normal), 1.0f);

	if (diffuseIntensity > 0.0f)
	{
		float3 reflected = reflect(-lightVec, normal);
		float specularIntensity = min(pow(max(dot(reflected, viewVector), 0.0f), material.Specular.w), 1.0f);

		result.Diffuse = diffuseIntensity * material.Diffuse * float4(light.Diffuse, 1.0f);
		result.Specular = specularIntensity * material.Specular * light.Specular;
	}

	return result;
}

LightingResult CalculateLighting(Light lights[MAX_LIGHTS], Material material, float3 position, float3 normal, float3 viewVector)
{
	LightingResult totalResult = InitLight();

	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		LightingResult result = InitLight();

		if (lights[i].Enabled == 1)
		{
			switch (lights[i].LightType)
			{
			case DIRECTIONAL:
				result = CalculateDirectional(lights[i], material, normal, viewVector);
				break;

			case POINT:
				result = CalculatePoint(lights[i], material, position, normal, viewVector);
				break;

			case SPOT:
				result = CalculateSpot(lights[i], material, position, normal, viewVector);
				break;
			}

			totalResult.Ambient += result.Ambient;
			totalResult.Diffuse += result.Diffuse;
			totalResult.Specular += result.Specular;
		}

		totalResult.Ambient = saturate(totalResult.Ambient);
		totalResult.Diffuse = saturate(totalResult.Diffuse);	//Clamp value in range 0 and 1 with saturate
		totalResult.Specular = saturate(totalResult.Specular);
	}

	return totalResult;
}