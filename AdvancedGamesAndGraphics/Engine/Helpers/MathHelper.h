#pragma once
#include "Engine\Structure\Singleton.h"

#include <DirectXMath.h>
#include <Windows.h>

class MathHelper
{
public:
	static DirectX::XMFLOAT4X4 Identity();

	static UINT Align(UINT uiLocation, UINT uiAlign);

protected:

private:

};

