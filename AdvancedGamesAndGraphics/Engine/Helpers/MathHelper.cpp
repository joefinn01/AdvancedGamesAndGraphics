#include "MathHelper.h"

DirectX::XMFLOAT4X4 MathHelper::Identity()
{
	return DirectX::XMFLOAT4X4(1, 0, 0, 0,
								0, 1, 0, 0,
								0, 0, 1, 0,
								0, 0, 0, 1);
}
