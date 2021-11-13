#pragma once

#include "Engine/Managers/LightManager.h"
#include "Engine/DirectX/Light.h"

#include <DirectXMath.h>

struct GBufferPerFrameCB
{
	DirectX::XMFLOAT4X4 ViewProjection;
};

struct LightPassPerFrameCB
{
	DirectX::XMFLOAT4X4 InvViewProjection;

	DirectX::XMFLOAT3 EyePosition;
	int ScreenWidth;

	int ScreenHeight;
	DirectX::XMFLOAT3 pad;
};

struct LightPassCB
{
	Light light;
};

struct VisibleGameObjectCB
{
	DirectX::XMFLOAT4X4 World;
	DirectX::XMFLOAT4X4 InvWorld;
};

struct MaterialCB
{
	DirectX::XMFLOAT4 Ambient;
	DirectX::XMFLOAT4 Diffuse;	//4th float is the alpha
	DirectX::XMFLOAT4 Specular;	//4th float is the specular power
};