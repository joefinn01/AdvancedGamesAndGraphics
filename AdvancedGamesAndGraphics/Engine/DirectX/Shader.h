#pragma once

#include "Engine/DirectX/UploadBuffer.h"

#include <wrl/client.h>
#include <d3d12.h>

struct ConstantBuffer;

template<class T>
class Shader
{
public:
	Shader(ID3DBlob* pShaderBlob, UploadBuffer<T>* pConstantBuffer)
	{
		m_pConstantUploadBuffer = pConstantBuffer;
		m_pShaderBlob = pShaderBlob;
	}

	~Shader()
	{
		if (m_pConstantUploadBuffer != nullptr)
		{
			delete m_pConstantUploadBuffer;

			m_pConstantUploadBuffer = nullptr;
		}

		m_pShaderBlob->Release();
	}

	UploadBuffer<T>* GetConstantUploadBuffer()
	{
		return m_pConstantUploadBuffer;
	}

	Microsoft::WRL::ComPtr<ID3DBlob> GetShaderBlob()
	{
		return m_pShaderBlob;
	}

protected:

private:
	Microsoft::WRL::ComPtr<ID3DBlob> m_pShaderBlob = nullptr;

	UploadBuffer<T>* m_pConstantUploadBuffer;

};

