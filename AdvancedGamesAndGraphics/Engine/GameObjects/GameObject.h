#pragma once

#include "Engine/Helpers/MathHelper.h"

#include <string>

class Timer;

enum class GameObjectType
{
	BASE = 0,
	VISIBLE,
	NUM_GAMEOBJECT_TYPES
};

class GameObject
{
public:
	GameObject();
	~GameObject();

	virtual bool Init(std::string sName, DirectX::XMFLOAT4 position, DirectX::XMFLOAT4 rotationQuat, DirectX::XMFLOAT4 scale);
	virtual void Update(const Timer& kTimer);
	virtual void Destroy();

	std::string GetName() const;

	GameObjectType GetType() const;

	void SetRotation(const DirectX::XMFLOAT4X4& rotationMatrix);
	void SetRotation(const DirectX::XMFLOAT4& rotationQuat);
	void SetRotation(float fRoll, float fPitch, float fYaw);

	void Rotate(const DirectX::XMFLOAT4X4& rotationMatrix);
	void Rotate(const DirectX::XMFLOAT4& rotationQuat);
	void Rotate(float fRoll, float fPitch, float fYaw);

	DirectX::XMFLOAT3 GetEulerAngles() const;
	DirectX::XMFLOAT4 GetOrientation() const;

	void SetPosition(DirectX::XMFLOAT4 position);
	void SetPosition(float fX, float fY, float fZ);
	void Translate(DirectX::XMFLOAT4 translation);
	void Translate(float fX, float fY, float fZ);

	DirectX::XMFLOAT4 GetPosition() const;

	void SetScale(DirectX::XMFLOAT4 scale);
	void SetScale(float fX, float fY, float fZ);
	void AdjustScale(DirectX::XMFLOAT4 scaleDifference);
	void AdjustScale(float fX, float fY, float fZ);

	DirectX::XMFLOAT4 GetScale() const;

	DirectX::XMFLOAT4 GetUpVector() const;
	DirectX::XMFLOAT4 GetForwardVector() const;
	DirectX::XMFLOAT4 GetRightVector() const;

protected:
	GameObjectType m_eType = GameObjectType::BASE;

private:
	void UpdateAxisVectors();

	std::string m_sName = "";

	DirectX::XMFLOAT4 m_Position = DirectX::XMFLOAT4();
	DirectX::XMFLOAT4X4 m_RotationMatrix = MathHelper::Identity();
	DirectX::XMFLOAT4 m_Scale = DirectX::XMFLOAT4(1, 1, 1, 0);

	DirectX::XMFLOAT4 m_Up = DirectX::XMFLOAT4(0, 1, 0, 0);
	DirectX::XMFLOAT4 m_Right = DirectX::XMFLOAT4(1, 0, 0, 0);
};

