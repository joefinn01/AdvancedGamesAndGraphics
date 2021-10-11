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
	Light()
	{
		Direction = DirectX::XMFLOAT3();
		FallOffStart = 0;

		Color = DirectX::XMFLOAT3();
		FallOffEnd = 0;

		Position = DirectX::XMFLOAT3();
		SpotLightPower = 0;

		InUse = false;
	}

	DirectX::XMFLOAT3 Direction;
	float FallOffStart;

	DirectX::XMFLOAT3 Color;
	float FallOffEnd;

	DirectX::XMFLOAT3 Position;
	float SpotLightPower;

	int InUse;
	int Type;
	DirectX::XMFLOAT2 pad;
};

struct PointLight : Light
{
	PointLight(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 color, float fFallOffStart, float fFallOffEnd)
	{
		Position = position;
		Color = color;
		FallOffStart = fFallOffStart;
		FallOffEnd = fFallOffEnd;
		InUse = true;
		Type = (int)LightType::POINT;
	}
};

struct SpotLight : Light
{
	SpotLight(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 color, float fFallOffStart, float fFallOffEnd, DirectX::XMFLOAT3 direction, float fSpotLightPower)
	{
		Position = position;
		Color = color;
		FallOffStart = fFallOffStart;
		FallOffEnd = fFallOffEnd;
		Direction = direction;
		SpotLightPower = fSpotLightPower;
		InUse = true;
		Type = (int)LightType::SPOT;
	}
};

struct DirectionalLight : Light
{
	DirectionalLight(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 direction)
	{
		Position = position;
		Color = color;
		Direction = direction;
		InUse = true;
		Type = (int)LightType::DIRECTIONAL;
	}
};