#pragma once
#include "Camera.h"
class DebugCamera : public Camera
{
public:
	DebugCamera(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 at, DirectX::XMFLOAT3 up, float fNearDepth, float fFarDepth, std::string sName);

	virtual void Update(const Timer& kTimer) override;

protected:
	bool m_bLockMouse = true;

	float m_fMouseSensitivity = 1.0f;

private:

};

