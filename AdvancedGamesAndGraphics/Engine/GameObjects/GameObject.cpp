#include "GameObject.h"
#include "Engine/Managers/ObjectManager.h"

using namespace DirectX;

GameObject::GameObject()
{
}

GameObject::~GameObject()
{
}

bool GameObject::Init(std::string sName, DirectX::XMFLOAT4 position, DirectX::XMFLOAT4 rotationQuat, DirectX::XMFLOAT4 scale)
{
	m_sName = sName;

	m_Position = position;

	XMStoreFloat4x4(&m_RotationMatrix, XMMatrixRotationQuaternion(XMLoadFloat4(&rotationQuat)));
	m_Scale = scale;

	m_eType = GameObjectType::BASE;

	UpdateAxisVectors();

	ObjectManager::GetInstance()->AddGameObject(this);

	return true;
}

void GameObject::Update(const Timer& kTimer)
{
}

void GameObject::Destroy()
{
	ObjectManager::GetInstance()->RemoveGameObject(m_sName);
}

std::string GameObject::GetName() const
{
	return m_sName;
}

GameObjectType GameObject::GetType() const
{
	return m_eType;
}

void GameObject::SetRotation(const DirectX::XMFLOAT4X4& rotationMatrix)
{
	m_RotationMatrix = rotationMatrix;

	UpdateAxisVectors();
}

void GameObject::SetRotation(const DirectX::XMFLOAT4& rotationQuat)
{
	XMStoreFloat4x4(&m_RotationMatrix, XMMatrixRotationQuaternion(XMLoadFloat4(&rotationQuat)));

	UpdateAxisVectors();
}

void GameObject::SetRotation(float fRoll, float fPitch, float fYaw)
{
	XMStoreFloat4x4(&m_RotationMatrix, XMMatrixRotationRollPitchYaw(fRoll, fYaw, fPitch));

	UpdateAxisVectors();
}

void GameObject::Rotate(const DirectX::XMFLOAT4X4& rotationMatrix)
{
	//Convert both the rotation matrices to quaternions before multiplying them
	XMStoreFloat4x4(&m_RotationMatrix, XMMatrixRotationQuaternion(XMQuaternionMultiply(XMQuaternionRotationMatrix(XMLoadFloat4x4(&m_RotationMatrix)), XMQuaternionRotationMatrix(XMLoadFloat4x4(&rotationMatrix)))));

	UpdateAxisVectors();
}

void GameObject::Rotate(const DirectX::XMFLOAT4& rotationQuat)
{
	XMStoreFloat4x4(&m_RotationMatrix, XMMatrixRotationQuaternion(XMQuaternionMultiply(XMQuaternionRotationMatrix(XMLoadFloat4x4(&m_RotationMatrix)), XMLoadFloat4(&rotationQuat))));

	UpdateAxisVectors();
}

void GameObject::Rotate(float fRoll, float fPitch, float fYaw)
{
	XMStoreFloat4x4(&m_RotationMatrix, XMMatrixRotationQuaternion(XMQuaternionMultiply(XMQuaternionRotationMatrix(XMLoadFloat4x4(&m_RotationMatrix)), XMQuaternionRotationRollPitchYaw(fPitch, fYaw, fRoll))));

	UpdateAxisVectors();
}

DirectX::XMFLOAT3 GameObject::GetEulerAngles() const
{
	float Roll;
	float Pitch;
	float Yaw;

	if (m_RotationMatrix._11 == 1.0f)
	{
		Yaw = atan2f(m_RotationMatrix._13, m_RotationMatrix._34);
		Pitch = 0.0f;
		Roll = 0.0f;

	}
	else if (m_RotationMatrix._11 == -1.0f)
	{
		Yaw = atan2f(m_RotationMatrix._13, m_RotationMatrix._34);
		Pitch = 0.0f;
		Roll = 0.0f;
	}
	else
	{

		Yaw = atan2f(-m_RotationMatrix._31, m_RotationMatrix._11);
		Pitch = asinf(m_RotationMatrix._21);
		Roll = atan2f(-m_RotationMatrix._23, m_RotationMatrix._22);
	}
	return XMFLOAT3(Pitch, Yaw, Roll);
}

DirectX::XMFLOAT4 GameObject::GetOrientation() const
{
	XMFLOAT4 quat;
	XMStoreFloat4(&quat, XMQuaternionRotationMatrix(XMLoadFloat4x4(&m_RotationMatrix)));

	return quat;
}

void GameObject::SetPosition(DirectX::XMFLOAT4 position)
{
	m_Position = position;
}

void GameObject::SetPosition(float fX, float fY, float fZ)
{
	m_Position = XMFLOAT4(fX, fY, fZ, 1);
}

void GameObject::Translate(DirectX::XMFLOAT4 translation)
{
	XMStoreFloat4(&m_Position, XMLoadFloat4(&m_Position) + XMLoadFloat4(&translation));
}

void GameObject::Translate(float fX, float fY, float fZ)
{
	XMStoreFloat4(&m_Position, XMLoadFloat4(&m_Position) + XMVectorSet(fX, fY, fZ, 0));
}

DirectX::XMFLOAT4 GameObject::GetPosition() const
{
	return m_Position;
}

void GameObject::SetScale(DirectX::XMFLOAT4 scale)
{
	m_Scale = scale;
}

void GameObject::SetScale(float fX, float fY, float fZ)
{
	m_Scale = XMFLOAT4(fX, fY, fZ, 1);
}

void GameObject::AdjustScale(DirectX::XMFLOAT4 scaleDifference)
{
	XMStoreFloat4(&m_Scale, XMLoadFloat4(&m_Scale) + XMLoadFloat4(&scaleDifference));
}

void GameObject::AdjustScale(float fX, float fY, float fZ)
{
	XMStoreFloat4(&m_Scale, XMLoadFloat4(&m_Scale) + XMVectorSet(fX, fY, fZ, 0));
}

DirectX::XMFLOAT4 GameObject::GetScale() const
{
	return m_Scale;
}

DirectX::XMFLOAT4 GameObject::GetUpVector() const
{
	return m_Up;
}

DirectX::XMFLOAT4 GameObject::GetForwardVector() const
{
	XMFLOAT4 forward;
	XMStoreFloat4(&forward, XMVector3Cross(XMLoadFloat4(&m_Up), XMLoadFloat4(&m_Right)));

	return forward;
}

DirectX::XMFLOAT4 GameObject::GetRightVector() const
{
	return m_Right;
}

void GameObject::UpdateAxisVectors()
{
	XMStoreFloat4(&m_Up, XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), XMLoadFloat4x4(&m_RotationMatrix)));
	XMStoreFloat4(&m_Right, XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), XMLoadFloat4x4(&m_RotationMatrix)));
}
