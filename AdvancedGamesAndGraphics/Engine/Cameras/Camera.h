#pragma once

#include "Engine/Helpers/MathHelper.h"

//#include <DirectXMath.h>
#include <string>

class Timer;

class Camera
{
public:
	Camera(DirectX::XMFLOAT4 position, DirectX::XMFLOAT4 at, DirectX::XMFLOAT4 up, float fNearDepth, float fFarDepth, std::string sName);
	Camera();
	~Camera();

	virtual void Update(const Timer& kTimer);

	void SetPosition(DirectX::XMFLOAT4 position);
	DirectX::XMFLOAT4 GetPosition() const;

	void Translate(DirectX::XMFLOAT4 vec);

	void SetLookAt(DirectX::XMFLOAT4 at);
	DirectX::XMFLOAT4 GetLookAt() const;

	void SetUpVector(DirectX::XMFLOAT4 up);
	DirectX::XMFLOAT4 GetUpVector() const;

	DirectX::XMFLOAT4 GetRightVector() const;
	DirectX::XMFLOAT4 GetForwardVector() const;

	DirectX::XMFLOAT4X4 GetViewMatrix() const;
	DirectX::XMFLOAT4X4 GetProjectionMatrix() const;
	DirectX::XMFLOAT4X4 GetViewProjectionMatrix() const;

	std::string GetName() const;

	void Reshape(float fNearDepth, float fFarDepth);

protected:
	DirectX::XMFLOAT4 m_Eye = DirectX::XMFLOAT4();
	DirectX::XMFLOAT4 m_At = DirectX::XMFLOAT4();

	DirectX::XMFLOAT4 m_Up = DirectX::XMFLOAT4(0, 1, 0, 0);
	DirectX::XMFLOAT4 m_Forward = DirectX::XMFLOAT4(0, 0, 1, 0);

	DirectX::XMFLOAT4X4 m_RotationMatrix = MathHelper::Identity();

	DirectX::XMFLOAT4X4 m_View = MathHelper::Identity();
	DirectX::XMFLOAT4X4 m_Projection = MathHelper::Identity();

	float m_fNearDepth;
	float m_fFarDepth;

	std::string m_sName = "";

private:

};

