#include "DebugCamera.h"
#include "Engine/Commons/Timer.h"
#include "Engine/Managers/WindowManager.h"
#include "Engine/Helpers/DebugHelper.h"

#include <Windows.h>

using namespace DirectX;

Tag tag = L"DebugCamera";

DebugCamera::DebugCamera(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 at, DirectX::XMFLOAT3 up, float fNearDepth, float fFarDepth, std::string sName) : Camera(position, at, up, fNearDepth, fFarDepth, sName)
{
	if (m_bLockMouse)
	{
		//center the mouse.
		SetCursorPos(WindowManager::GetInstance()->GetWindowWidth() / 2, WindowManager::GetInstance()->GetWindowHeight() / 2);
	}

}

void DebugCamera::Update(const Timer& kTimer)
{
	if (GetAsyncKeyState(VK_UP) || GetAsyncKeyState(0x57))	//W
	{
		XMStoreFloat3(&m_Eye, XMLoadFloat3(&m_Eye) + XMLoadFloat3(&GetForwardVector()) * kTimer.DeltaTime() * 25.0f);
	}
	if (GetAsyncKeyState(VK_DOWN) || GetAsyncKeyState(0x53)) //S
	{
		XMStoreFloat3(&m_Eye, XMLoadFloat3(&m_Eye) + XMLoadFloat3(&GetForwardVector()) * kTimer.DeltaTime() * -25.0f);
	}
	if (GetAsyncKeyState(VK_LEFT) || GetAsyncKeyState(0x41)) //A
	{
		XMStoreFloat3(&m_Eye, XMLoadFloat3(&m_Eye) + XMLoadFloat3(&GetRightVector()) * kTimer.DeltaTime() * 25.0f);
	}
	if (GetAsyncKeyState(VK_RIGHT) || GetAsyncKeyState(0x44))	//D
	{
		XMStoreFloat3(&m_Eye, XMLoadFloat3(&m_Eye) + XMLoadFloat3(&GetRightVector()) * kTimer.DeltaTime() * -25.0f);
	}

	//Toggle the mouse being locked to the screen if escape is pressed
	if (GetAsyncKeyState(VK_CONTROL))
	{
		m_bLockMouse = !m_bLockMouse;

		//If toggling on then center the mouse so there isn't a big change in mouse position when it's first toggled on.
		if (m_bLockMouse == true)
		{
			SetCursorPos(WindowManager::GetInstance()->GetWindowWidth() / 2, WindowManager::GetInstance()->GetWindowHeight() / 2);
		}
	}

	if (m_bLockMouse == true)
	{
		POINT mousePosition;
		GetCursorPos(&mousePosition);

		int iCentreX = WindowManager::GetInstance()->GetWindowWidth() / 2;
		int iCentreY = WindowManager::GetInstance()->GetWindowHeight() / 2;

		XMFLOAT2 deltaMousePosition = XMFLOAT2(iCentreX - mousePosition.x, iCentreY - mousePosition.y);

		//Calculate the change in yaw
		XMMATRIX rotationMatrix = XMMatrixRotationY(deltaMousePosition.x * kTimer.DeltaTime() * -m_fMouseSensitivity);

		XMStoreFloat3(&m_Forward, XMVector3TransformNormal(XMLoadFloat3(&m_Forward), rotationMatrix));
		XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMLoadFloat3(&m_Up), rotationMatrix));

		XMFLOAT3 right = GetRightVector();

		//Calculate the change in pitch
		rotationMatrix = XMMatrixRotationAxis(XMLoadFloat3(&right), deltaMousePosition.y * kTimer.DeltaTime() * m_fMouseSensitivity);

		XMStoreFloat3(&m_Forward, XMVector3TransformNormal(XMVector3Normalize(XMLoadFloat3(&m_Forward)), rotationMatrix));
		XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMVector3Normalize(XMLoadFloat3(&m_Up)), rotationMatrix));

		//Recenter the mouse for the next frame.
		SetCursorPos(iCentreX, iCentreY);
	}

	Camera::Update(kTimer);
}
