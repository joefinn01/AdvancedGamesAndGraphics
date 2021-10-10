#pragma once

#include <DirectXMath.h>

struct PerFrameCB
{
	DirectX::XMMATRIX ViewProjection;
	DirectX::XMMATRIX InvTransposeViewProjection;
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