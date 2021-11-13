#pragma once

#include "Engine/DirectX/ConstantBuffers.h"

#include <DirectXMath.h>

enum class LightType
{
	SPOT = 0,
	POINT,
	DIRECTIONAL
};

struct Light
{
	LightCB lightCB;

	int Index;
	bool Enabled;
};

struct PointLight : Light
{
	PointLight(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 ambient, DirectX::XMFLOAT3 diffuse, DirectX::XMFLOAT4 specular, DirectX::XMFLOAT3 attenuation, float fRange, bool bEnabled = true)
	{
		lightCB.Position = position;
		lightCB.Ambient = ambient;
		lightCB.Diffuse = diffuse;
		lightCB.Specular = specular;
		lightCB.Attenuation = attenuation;
		lightCB.Range = fRange;
		lightCB.Type = (int)LightType::POINT;

		Enabled = bEnabled;
	}
};

struct SpotLight : Light
{
	SpotLight(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 ambient, DirectX::XMFLOAT3 diffuse, DirectX::XMFLOAT4 specular, DirectX::XMFLOAT3 attenuation, float fRange, DirectX::XMFLOAT4 direction, float fSpotLightAngle, bool bEnabled = true)
	{
		lightCB.Position = position;
		lightCB.Ambient = ambient;
		lightCB.Diffuse = diffuse;
		lightCB.Specular = specular;
		lightCB.Attenuation = attenuation;
		lightCB.Range = fRange;
		lightCB.Direction = direction;
		lightCB.SpotLightAngle = fSpotLightAngle;
		lightCB.Type = (int)LightType::SPOT;

		Enabled = bEnabled;
	}
};

struct DirectionalLight : Light
{
	DirectionalLight(DirectX::XMFLOAT3 ambient, DirectX::XMFLOAT3 diffuse, DirectX::XMFLOAT4 specular, DirectX::XMFLOAT4 direction, bool bEnabled = true)
	{
		lightCB.Ambient = ambient;
		lightCB.Diffuse = diffuse;
		lightCB.Specular = specular;
		lightCB.Direction = direction;
		lightCB.Type = (int)LightType::DIRECTIONAL;
		
		Enabled = bEnabled;
	}
};