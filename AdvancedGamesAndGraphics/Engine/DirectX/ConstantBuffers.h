#pragma once

#include "Engine/Managers/LightManager.h"
#include "Engine/DirectX/Light.h"

#include <DirectXMath.h>

struct PerFrameCB
{
	DirectX::XMMATRIX ViewProjection;
	DirectX::XMMATRIX InvTransposeViewProjection;

	Light Lights[MAX_LIGHTS];

	DirectX::XMFLOAT4 Ambient;

	DirectX::XMFLOAT3 EyePosition;
	float pad;
};

struct GameObjectCB
{
	DirectX::XMMATRIX World;
};

struct VisibleGameObjectCB
{
	DirectX::XMMATRIX World;
};

struct MaterialCB
{
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT3 fresnel;
	float roughness;
};