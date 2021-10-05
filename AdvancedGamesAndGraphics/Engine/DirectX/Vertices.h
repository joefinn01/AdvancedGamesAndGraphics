#pragma once

#include <DirectXMath.h>

struct Vertex
{
	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT4 color;

	Vertex(DirectX::XMFLOAT4 pos, DirectX::XMFLOAT4 col)
	{
		position = pos;
		color = col;
	}
};