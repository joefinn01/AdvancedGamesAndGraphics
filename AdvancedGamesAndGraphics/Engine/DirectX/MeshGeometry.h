#pragma once

#include <wrl/client.h>
#include <d3d12.h>
#include <string>
#include <unordered_map>

struct SubMeshGeometry
{
	UINT m_uiIndexCount = 0;
	UINT m_uiStartIndexLocation = 0;
	UINT m_uiStartVertexLocation = 0;
};

struct MeshGeometry
{
	//Memory references
	Microsoft::WRL::ComPtr<ID3DBlob> m_pVertexBufferCPU = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> m_pIndexBufferCPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexBufferGPU = nullptr;

	//Uploaders
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexBufferUploader = nullptr;

	std::string m_sName = "";

	//Data information
	UINT m_uiVertexByteStride = 0;
	UINT m_uiVertexByteSize = 0;
	UINT m_uiIndexByteSize = 0;

	DXGI_FORMAT m_IndexFormat = DXGI_FORMAT_R16_UINT;

	std::unordered_map<std::string, SubMeshGeometry> m_SubMeshes;

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
	{
		D3D12_VERTEX_BUFFER_VIEW vbView;
		vbView.BufferLocation = m_pVertexBufferGPU->GetGPUVirtualAddress();
		vbView.StrideInBytes = m_uiVertexByteStride;
		vbView.SizeInBytes = m_uiVertexByteSize;

		return vbView;
	}

	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
	{
		D3D12_INDEX_BUFFER_VIEW ibView;
		ibView.BufferLocation = m_pIndexBufferGPU->GetGPUVirtualAddress();
		ibView.SizeInBytes = m_uiIndexByteSize;
		ibView.Format = m_IndexFormat;

		return ibView;
	}
};