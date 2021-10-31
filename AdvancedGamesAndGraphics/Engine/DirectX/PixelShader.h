#pragma once
#include "Engine/DirectX/Shader.h"

template<class T>
class PixelShader : Shader<T>
{
public:
	PixelShader(ID3DBlob* pShaderBlob, UploadBuffer<T>* pConstantBuffer) : Shader<T>(pShaderBlob, pConstantBuffer)
	{
		m_eShaderType = ShaderType::Pixel;
	}
};