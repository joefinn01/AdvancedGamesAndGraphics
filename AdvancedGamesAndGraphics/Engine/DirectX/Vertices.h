#pragma once

#include <DirectXMath.h>

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;

	Vertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 norm)
	{
		position = pos;
		normal = norm;
	}

	Vertex(DirectX::XMFLOAT3 pos)
	{
		position = pos;
		normal = DirectX::XMFLOAT3(0, 0, 0);
	}

	Vertex()
	{
		position = DirectX::XMFLOAT3(0, 0, 0);
		normal = DirectX::XMFLOAT3(0, 0, 0);
	}
};