#pragma once

#include <string>
#include <Windows.h>
#include <DirectXMath.h>

struct Material
{
	std::string name;
	UINT CBIndex;
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT3 fresnel;
	float roughness;
};