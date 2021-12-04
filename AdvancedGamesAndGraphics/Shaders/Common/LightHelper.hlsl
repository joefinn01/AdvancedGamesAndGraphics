
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

Material InitMaterial(float4 ambient, float4 diffuse, float4 specular)
{
	Material mat;
	mat.Ambient = ambient;
	mat.Diffuse = diffuse;
	mat.Specular = specular;
	
	return mat;
}

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
    float pad;

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

	result.Ambient = saturate(result.Ambient);
	result.Diffuse = saturate(result.Diffuse);
	result.Specular = saturate(result.Specular);
	
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

	result.Ambient = saturate(result.Ambient);
	result.Diffuse = saturate(result.Diffuse);
	result.Specular = saturate(result.Specular);
	
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

	result.Ambient = saturate(result.Ambient);
	result.Diffuse = saturate(result.Diffuse);
	result.Specular = saturate(result.Specular);
	
	return result;
}