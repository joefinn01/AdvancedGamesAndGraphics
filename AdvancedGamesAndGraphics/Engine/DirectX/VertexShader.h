#pragma once
#include "Engine/DirectX/Shader.h"

template<class T>
class VertexShader : Shader<T>
{
public:
	VertexShader(ID3DBlob* pShaderBlob, UploadBuffer<T>* pConstantBuffer) : Shader<T>(pShaderBlob, pConstantBuffer)
	{
		m_eShaderType = ShaderType::Vertex;
	}
};