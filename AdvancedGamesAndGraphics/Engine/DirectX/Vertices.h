#pragma once

#include <DirectXMath.h>

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 color;
	DirectX::XMFLOAT3 normal;

	Vertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT4 col, DirectX::XMFLOAT3 norm)
	{
		position = pos;
		color = col;
		normal = norm;
	}

	Vertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT4 col)
	{
		position = pos;
		color = col;
		normal = DirectX::XMFLOAT3(0, 0, 0);
	}

	Vertex()
	{
		position = DirectX::XMFLOAT3(0, 0, 0);
		color = DirectX::XMFLOAT4(0, 0, 0, 0);
		normal = DirectX::XMFLOAT3(0, 0, 0);
	}
};