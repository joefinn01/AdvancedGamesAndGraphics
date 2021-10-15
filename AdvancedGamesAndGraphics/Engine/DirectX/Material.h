#pragma once

#include <string>
#include <Windows.h>
#include <DirectXMath.h>

struct Material
{
	std::string name;
	UINT CBIndex;

	DirectX::XMFLOAT4 Ambient;
	DirectX::XMFLOAT4 Diffuse;	//4th float is the alpha
	DirectX::XMFLOAT4 Specular;	//4th float is the specular power
};