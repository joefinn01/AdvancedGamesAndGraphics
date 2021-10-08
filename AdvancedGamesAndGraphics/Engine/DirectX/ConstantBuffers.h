#pragma once

#include <DirectXMath.h>

struct PerFrameCB
{
	DirectX::XMMATRIX ViewProjection;
};

struct GameObjectCB
{
	DirectX::XMMATRIX World;
};

struct VisibleGameObjectCB
{
	DirectX::XMMATRIX World;
};