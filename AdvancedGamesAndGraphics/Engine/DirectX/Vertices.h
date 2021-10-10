#pragma once

#include <DirectXMath.h>

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 color;

	Vertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT4 col)
	{
		position = pos;
		color = col;
	}
};