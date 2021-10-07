#pragma once

#include <DirectXMath.h>

struct ConstantBuffer
{

};

struct PerFrameCB
{
	DirectX::XMMATRIX ViewProjection;
};

struct GameObjectCB : ConstantBuffer
{
	DirectX::XMMATRIX World;
};

struct VisibleGameObjectCB : GameObjectCB
{

};