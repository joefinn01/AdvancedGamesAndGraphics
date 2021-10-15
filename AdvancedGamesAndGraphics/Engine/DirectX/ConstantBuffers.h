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
	DirectX::XMMATRIX InvWorld;
};

struct MaterialCB
{
	DirectX::XMFLOAT4 Ambient;
	DirectX::XMFLOAT4 Diffuse;	//4th float is the alpha
	DirectX::XMFLOAT4 Specular;	//4th float is the specular power
};