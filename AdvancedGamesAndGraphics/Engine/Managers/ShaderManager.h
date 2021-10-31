#pragma once
#include "Engine\Structure\Singleton.h"
#include "Engine/DirectX/Shader.h"
#include "Engine/DirectX/VertexShader.h"
#include "Engine/DirectX/PixelShader.h"

#include <unordered_map>
#include <wrl/client.h>
#include <d3d12.h>
#include <d3dcompiler.h>

struct ConstantBuffer;

class ShaderManager : public Singleton<ShaderManager>
{
public:
	template<class T>
	Microsoft::WRL::ComPtr<ID3DBlob> CompileShaderVS(const std::wstring& kwsFilename, const std::string& ksName, const D3D_SHADER_MACRO* pDefines, const std::string& ksEntrypoint, const std::string& ksTarget, UploadBuffer<T>* pConstantUploadBuffer)
	{
		if (m_Shaders.count(ksName) == 1)
		{
			LOG_ERROR(tag, L"Tried to call a newly compiled shader %s but one with that name already exists!", ksName);

			return nullptr;
		}

		UINT compileFlags = 0;

#if _DEBUG  
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		Microsoft::WRL::ComPtr<ID3DBlob> pByteCode = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> pErrors;
		HRESULT hr = D3DCompileFromFile(kwsFilename.c_str(),
			pDefines,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			ksEntrypoint.c_str(),
			ksTarget.c_str(),
			compileFlags,
			0,
			&pByteCode,
			&pErrors);

		if (pErrors != nullptr)
		{
			OutputDebugStringA((char*)pErrors->GetBufferPointer());
		}


		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to compile shader %ls!", kwsFilename);

			return nullptr;
		}

		m_Shaders[ksName] = static_cast<void*>(new VertexShader<T>(pByteCode.Get(), pConstantUploadBuffer));

		return pByteCode;
	}

	template<class T>
	Microsoft::WRL::ComPtr<ID3DBlob> CompileShaderPS(const std::wstring& kwsFilename, const std::string& ksName, const D3D_SHADER_MACRO* pDefines, const std::string& ksEntrypoint, const std::string& ksTarget, UploadBuffer<T>* pConstantUploadBuffer)
	{
		if (m_Shaders.count(ksName) == 1)
		{
			LOG_ERROR(tag, L"Tried to call a newly compiled shader %s but one with that name already exists!", ksName);

			return nullptr;
		}

		UINT compileFlags = 0;

#if _DEBUG  
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		Microsoft::WRL::ComPtr<ID3DBlob> pByteCode = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> pErrors;
		HRESULT hr = D3DCompileFromFile(kwsFilename.c_str(),
			pDefines,
			D3D_COMPILE_STANDARD_FILE_INCLUDE,
			ksEntrypoint.c_str(),
			ksTarget.c_str(),
			compileFlags,
			0,
			&pByteCode,
			&pErrors);

		if (pErrors != nullptr)
		{
			OutputDebugStringA((char*)pErrors->GetBufferPointer());
		}


		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to compile shader %ls!", kwsFilename);

			return nullptr;
		}

		m_Shaders[ksName] = static_cast<void*>(new PixelShader<T>(pByteCode.Get(), pConstantUploadBuffer));

		return pByteCode;
	}

	template<class T>
	Shader<T>* GetShader(std::string sName)
	{
		if (m_Shaders.count(sName) == 0)
		{
			LOG_ERROR(tag, L"Tried to get a shader called %s but that shader doesn't exist!", sName);

			return nullptr;
		}

		return static_cast<Shader<T>*>(m_Shaders[sName]);
	}

	std::unordered_map<std::string, void*>* GetShaders()
	{
		return &m_Shaders;
	}

	template<class T>
	UploadBuffer<T>* GetShaderConstantUploadBuffer(std::string sName)
	{
		if (m_Shaders.count(sName) == 0)
		{
			LOG_ERROR(tag, L"Tried to get a constant buffer for a shader called %s but that shader doesn't exist!", sName);

			return nullptr;
		}

		return static_cast<Shader<T>*>(m_Shaders[sName])->GetConstantUploadBuffer();
	}

	bool RemoveShader(std::string sName);

protected:

private:
	std::unordered_map<std::string, void*> m_Shaders;

	Tag tag = L"ShaderManager";
};

