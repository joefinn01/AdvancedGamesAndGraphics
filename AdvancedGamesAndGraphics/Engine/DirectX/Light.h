#pragma once

#include <DirectXMath.h>

struct Light
{
	DirectX::XMFLOAT3 Direction;
	float FallOffStart;

	DirectX::XMFLOAT3 Color;
	float FallOffEnd;

	DirectX::XMFLOAT3 Position;
	float SpotLightPower;
};

struct PointLight : Light
{
	PointLight(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 color, float fFallOffStart, float fFallOffEnd)
	{
		Position = position;
		Color = color;
		FallOffStart = fFallOffStart;
		FallOffEnd = fFallOffEnd;
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
	}
};

struct DirectionalLight : Light
{
	DirectionalLight(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 color, DirectX::XMFLOAT3 direction)
	{
		Position = position;
		Color = color;
		Direction = direction;
	}
};