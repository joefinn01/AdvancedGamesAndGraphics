#pragma once

#include "Engine/Managers/LightManager.h"

#include <DirectXMath.h>

struct GBufferPerFrameCB
{
	DirectX::XMFLOAT4X4 ViewProjection;

	DirectX::XMFLOAT3 EyePosW;
	float pad;
};

struct LightPassPerFrameCB
{
	DirectX::XMFLOAT4X4 InvViewProjection;

	DirectX::XMFLOAT3 EyePosition;
	float pad;
};

struct PostProcessingPerFrameCB
{
	int ScreenWidth;
	int ScreenHeight;

	int Enabled;
	int BoxBlurNumber;
};

struct ScreenSpaceAOCB
{
	DirectX::XMFLOAT4 RandomRotations[16];

	DirectX::XMFLOAT4 HemisphereSamples[64];
};

struct SSAOPerFrameCB
{
	DirectX::XMFLOAT4X4 View;
	DirectX::XMFLOAT4X4 InvProjection;
	DirectX::XMFLOAT4X4 ProjTex;

	int ScreenWidth;
	int ScreenHeight;
	int NumSamples;
	float Radius;
};

struct LightCB
{
	DirectX::XMFLOAT3 Position;
	float Range;

	DirectX::XMFLOAT4 Direction;

	DirectX::XMFLOAT3 Ambient;
	int Type;

	DirectX::XMFLOAT3 Diffuse;
	float pad;

	DirectX::XMFLOAT4 Specular;

	DirectX::XMFLOAT3 Attenuation;
	float SpotLightAngle;
};

struct VisibleGameObjectCB
{
	DirectX::XMFLOAT4X4 World;
	DirectX::XMFLOAT4X4 InvWorld;
};

struct MaterialCB
{
	DirectX::XMFLOAT4 Ambient;
	DirectX::XMFLOAT4 Diffuse;	//4th float is the alpha
	DirectX::XMFLOAT4 Specular;	//4th float is the specular power
};