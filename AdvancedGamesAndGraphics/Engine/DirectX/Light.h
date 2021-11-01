#pragma once

#include <DirectXMath.h>

enum class LightType
{
	SPOT = 0,
	POINT,
	DIRECTIONAL
};

struct Light
{
	DirectX::XMFLOAT3 Position;
	float Range;

	DirectX::XMFLOAT4 Direction;
	
	DirectX::XMFLOAT3 Ambient;
	int Type;

	DirectX::XMFLOAT3 Diffuse;
	int Enabled;

	DirectX::XMFLOAT4 Specular;

	DirectX::XMFLOAT3 Attenuation;
	float SpotLightAngle;
};

struct PointLight : Light
{
	PointLight(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 ambient, DirectX::XMFLOAT3 diffuse, DirectX::XMFLOAT4 specular, DirectX::XMFLOAT3 attenuation, float fRange, bool bEnabled = true)
	{
		Position = position;
		Ambient = ambient;
		Diffuse = diffuse;
		Specular = specular;
		Attenuation = attenuation;
		Range = fRange;
		Enabled = (int)bEnabled;
		Type = (int)LightType::POINT;
	}
};

struct SpotLight : Light
{
	SpotLight(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 ambient, DirectX::XMFLOAT3 diffuse, DirectX::XMFLOAT4 specular, DirectX::XMFLOAT3 attenuation, float fRange, DirectX::XMFLOAT4 direction, float fSpotLightAngle, bool bEnabled = true)
	{
		Position = position;
		Ambient = ambient;
		Diffuse = diffuse;
		Specular = specular;
		Attenuation = attenuation;
		Range = fRange;
		Direction = direction;
		SpotLightAngle = fSpotLightAngle;
		Enabled = 1;
		Type = (int)LightType::SPOT;
	}
};

struct DirectionalLight : Light
{
	DirectionalLight(DirectX::XMFLOAT3 ambient, DirectX::XMFLOAT3 diffuse, DirectX::XMFLOAT4 specular, DirectX::XMFLOAT4 direction, bool bEnabled = true)
	{
		Ambient = ambient;
		Diffuse = diffuse;
		Specular = specular;
		Direction = direction;
		Enabled = 1;
		Type = (int)LightType::DIRECTIONAL;
	}
};