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

//Have a second struct which holds only the needed information
struct MaterialConstants
{
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT3 fresnel;
	float roughness;
};