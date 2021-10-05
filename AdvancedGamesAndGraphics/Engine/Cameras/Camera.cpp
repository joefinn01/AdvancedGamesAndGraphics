#include "Camera.h"
#include "Engine/Managers/WindowManager.h"
#include "Engine/Commons\Timer.h"

using namespace DirectX;

Camera::Camera(DirectX::XMFLOAT4 position, DirectX::XMFLOAT4 at, DirectX::XMFLOAT4 up, float fNearDepth, float fFarDepth, std::string sName)
{
	m_Eye = position;
	m_At = at;
	m_Up = up;

	m_sName = sName;

	XMStoreFloat4(&m_Forward, XMLoadFloat4(&m_At) - XMLoadFloat4(&m_Eye));

	XMStoreFloat4x4(&m_View, XMMatrixLookToLH(XMLoadFloat4(&m_Eye), XMLoadFloat4(&m_Forward), XMLoadFloat4(&m_Up)));

	Reshape(fNearDepth, fFarDepth);
}

Camera::Camera()
{
	XMStoreFloat4x4(&m_View, XMMatrixLookToLH(XMLoadFloat4(&m_Eye), XMLoadFloat4(&m_Forward), XMLoadFloat4(&m_Up)));
}

Camera::~Camera()
{
}

void Camera::Update(const Timer& kTimer)
{
	XMStoreFloat4x4(&m_View, XMMatrixLookToLH(XMLoadFloat4(&m_Eye), XMLoadFloat4(&m_Forward), XMLoadFloat4(&m_Up)));
}

void Camera::SetPosition(DirectX::XMFLOAT4 position)
{
	m_Eye = position;
}

DirectX::XMFLOAT4 Camera::GetPosition() const
{
	return m_Eye;
}

void Camera::Translate(DirectX::XMFLOAT4 vec)
{
	XMStoreFloat4(&m_Eye, XMLoadFloat4(&m_Eye) + XMLoadFloat4(&vec));
}

void Camera::SetLookAt(DirectX::XMFLOAT4 at)
{
	m_At = at;
}

DirectX::XMFLOAT4 Camera::GetLookAt() const
{
	return m_At;
}

void Camera::SetUpVector(DirectX::XMFLOAT4 up)
{
	m_Up = up;
}

DirectX::XMFLOAT4 Camera::GetUpVector() const
{
	return m_Up;
}

DirectX::XMFLOAT4 Camera::GetRightVector() const
{
	XMFLOAT4 right;

	XMStoreFloat4(&right, XMVector3Cross(XMLoadFloat4(&m_Forward), XMLoadFloat4(&m_Up)));

	return right;
}

DirectX::XMFLOAT4 Camera::GetForwardVector() const
{
	return m_Forward;
}

DirectX::XMFLOAT4X4 Camera::GetViewMatrix() const
{
	return m_View;
}

DirectX::XMFLOAT4X4 Camera::GetProjectionMatrix() const
{
	return m_Projection;
}

DirectX::XMFLOAT4X4 Camera::GetViewProjectionMatrix() const
{
	XMFLOAT4X4 viewProj;

	XMStoreFloat4x4(&viewProj, XMMatrixMultiply(XMLoadFloat4x4(&m_View), XMLoadFloat4x4(&m_Projection)));

	return viewProj;
}

std::string Camera::GetName() const
{
	return m_sName;
}

void Camera::Reshape(float fNearDepth, float fFarDepth)
{
	m_fNearDepth = fNearDepth;
	m_fFarDepth = fFarDepth;

	XMStoreFloat4x4(&m_Projection, XMMatrixPerspectiveFovLH(XM_PIDIV2, WindowManager::GetInstance()->GetAspectRatio(), m_fNearDepth, m_fFarDepth));
}
