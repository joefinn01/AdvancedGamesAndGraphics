#pragma once
#include "Camera.h"
class DebugCamera : public Camera
{
public:
	DebugCamera(DirectX::XMFLOAT4 position, DirectX::XMFLOAT4 at, DirectX::XMFLOAT4 up, float fNearDepth, float fFarDepth, std::string sName);

	virtual void Update(const Timer& kTimer) override;

protected:

private:

};

