#include "MathHelper.h"

DirectX::XMFLOAT4X4 MathHelper::Identity()
{
	return DirectX::XMFLOAT4X4(1, 0, 0, 0,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1);
}

UINT MathHelper::Align(UINT uiLocation, UINT uiAlign)
{
	return (uiLocation + (uiAlign - 1)) & ~(uiAlign - 1);
}
