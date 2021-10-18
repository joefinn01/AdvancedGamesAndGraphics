#pragma once

#include <DirectXMath.h>

struct Vertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexCoords;
	DirectX::XMFLOAT3 Tangent;
	DirectX::XMFLOAT3 BiTangent;

	Vertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 norm, DirectX::XMFLOAT2 texCoords)
	{
		Position = pos;
		Normal = norm;
		TexCoords = texCoords;
		Tangent = DirectX::XMFLOAT3(0, 0, 0);
		BiTangent = DirectX::XMFLOAT3(0, 0, 0);
	}

	Vertex(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 norm)
	{
		Position = pos;
		Normal = norm;
		TexCoords = DirectX::XMFLOAT2(0, 0);
		Tangent = DirectX::XMFLOAT3(0, 0, 0);
		BiTangent = DirectX::XMFLOAT3(0, 0, 0);
	}

	Vertex(DirectX::XMFLOAT3 pos)
	{
		Position = pos;
		Normal = DirectX::XMFLOAT3(0, 0, 0);
		TexCoords = DirectX::XMFLOAT2(0, 0);
		Tangent = DirectX::XMFLOAT3(0, 0, 0);
		BiTangent = DirectX::XMFLOAT3(0, 0, 0);
	}

	Vertex()
	{
		Position = DirectX::XMFLOAT3(0, 0, 0);
		Normal = DirectX::XMFLOAT3(0, 0, 0);
		TexCoords = DirectX::XMFLOAT2(0, 0);
		Tangent = DirectX::XMFLOAT3(0, 0, 0);
		BiTangent = DirectX::XMFLOAT3(0, 0, 0);
	}
};