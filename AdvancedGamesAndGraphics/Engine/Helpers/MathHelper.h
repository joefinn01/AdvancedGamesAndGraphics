#pragma once
#include "Engine\Structure\Singleton.h"

#include <DirectXMath.h>

class MathHelper : public Singleton<MathHelper>
{
public:
	static DirectX::XMFLOAT4X4 Identity();

protected:

private:

};

