#include "DebugCamera.h"
#include "Engine/Commons/Timer.h"

#include <Windows.h>

using namespace DirectX;

DebugCamera::DebugCamera(DirectX::XMFLOAT4 position, DirectX::XMFLOAT4 at, DirectX::XMFLOAT4 up, float fNearDepth, float fFarDepth, std::string sName) : Camera(position, at, up, fNearDepth, fFarDepth, sName)
{
}

void DebugCamera::Update(const Timer& kTimer)
{
	if (GetAsyncKeyState(VK_UP) || GetAsyncKeyState(0x57))	//W
	{
		XMStoreFloat4(&m_Eye, XMLoadFloat4(&m_Eye) + XMLoadFloat4(&GetForwardVector()) * kTimer.DeltaTime() * 25.0f);
	}
	if (GetAsyncKeyState(VK_DOWN) || GetAsyncKeyState(0x53)) //S
	{
		XMStoreFloat4(&m_Eye, XMLoadFloat4(&m_Eye) + XMLoadFloat4(&GetForwardVector()) * kTimer.DeltaTime() * -25.0f);
	}
	if (GetAsyncKeyState(VK_LEFT) || GetAsyncKeyState(0x41)) //A
	{
		XMStoreFloat4(&m_Eye, XMLoadFloat4(&m_Eye) + XMLoadFloat4(&GetRightVector()) * kTimer.DeltaTime() * 25.0f);
	}
	if (GetAsyncKeyState(VK_RIGHT) || GetAsyncKeyState(0x44))	//D
	{
		XMStoreFloat4(&m_Eye, XMLoadFloat4(&m_Eye) + XMLoadFloat4(&GetRightVector()) * kTimer.DeltaTime() * -25.0f);
	}

	Camera::Update(kTimer);
}
