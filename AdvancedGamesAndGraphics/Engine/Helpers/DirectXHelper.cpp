#include "DirectXHelper.h"
#include "Engine/Helpers/DebugHelper.h"

#include <DirectX\d3dx12.h>
#include <d3dcompiler.h>

Tag tag = "DirectXHelper";

Microsoft::WRL::ComPtr<ID3D12Resource> DirectXHelper::CreateDefaultBuffer(ID3D12Device4* pDevice, ID3D12GraphicsCommandList* pGraphicsCommandList, const void* pData, UINT64 uiByteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& pUploadBuffer)
{
	Microsoft::WRL::ComPtr<ID3D12Resource> pDefaultBuffer;

	HRESULT hr = pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uiByteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(pDefaultBuffer.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, "Failed to create the default buffer!");

		return nullptr;
	}

	hr = pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uiByteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(pUploadBuffer.GetAddressOf()));


	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = pData;
	subResourceData.RowPitch = static_cast<LONG_PTR>(uiByteSize);
	subResourceData.SlicePitch = subResourceData.RowPitch;

	pGraphicsCommandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(pDefaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST));

	UpdateSubresources<1>(pGraphicsCommandList,
		pDefaultBuffer.Get(),
		pUploadBuffer.Get(),
		0,
		0,
		1,
		&subResourceData);

	pGraphicsCommandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(pDefaultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ));

	return pDefaultBuffer;
}

Microsoft::WRL::ComPtr<ID3DBlob> DirectXHelper::CompileShader(const std::wstring& kwsFilename, const std::string& ksName, const D3D_SHADER_MACRO* pDefines, const std::string& ksEntrypoint, const std::string& ksTarget)
{
	UINT compileFlags = 0;

#if _DEBUG  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	Microsoft::WRL::ComPtr<ID3DBlob> pByteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrors;
	hr = D3DCompileFromFile(kwsFilename.c_str(),
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
		LOG_ERROR(tag, "Failed to compile shader %S!", kwsFilename.c_str());

		return nullptr;
	}

	return pByteCode;
}
