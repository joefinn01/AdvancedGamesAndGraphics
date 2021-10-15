#pragma once

#include <DirectXMath.h>

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 TexCoords;

	Vertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 norm, DirectX::XMFLOAT2 texCoords)
	{
		position = pos;
		normal = norm;
		TexCoords = texCoords;
	}

	Vertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 norm)
	{
		position = pos;
		normal = norm;
		TexCoords = DirectX::XMFLOAT2(0, 0);
	}

	Vertex(DirectX::XMFLOAT3 pos)
	{
		position = pos;
		normal = DirectX::XMFLOAT3(0, 0, 0);
		TexCoords = DirectX::XMFLOAT2(0, 0);
	}

	Vertex()
	{
		position = DirectX::XMFLOAT3(0, 0, 0);
		normal = DirectX::XMFLOAT3(0, 0, 0);
		TexCoords = DirectX::XMFLOAT2(0, 0);
	}
};