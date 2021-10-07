#pragma once

#include <wrl.h>
#include <DirectX/d3dx12.h>
#include <string>

class DirectXHelper
{
public:
	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device4* pDevice, ID3D12GraphicsCommandList* pGraphicsCommandList, const void* pData, UINT64 uiByteSize, Microsoft::WRL::ComPtr<ID3D12Resource>& pUploadBuffer);

protected:

private:

};

