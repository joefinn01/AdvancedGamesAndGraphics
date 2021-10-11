#include "VisibleGameObject.h"
#include "Engine/Apps/App.h"
#include "Engine/DirectX/Vertices.h"
#include "Engine/DirectX/MeshGeometry.h"
#include "Engine/Helpers/DebugHelper.h"
#include "Engine/Helpers/DirectXHelper.h"
#include "Engine/Managers/MaterialManager.h"

#include <DirectX\d3dx12.h>
#include <DirectXColors.h>
#include <array>
#include <d3dcompiler.h>

using namespace DirectX;

Tag tag = L"VisibleGameObject";

bool VisibleGameObject::Init(std::string sName, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation, DirectX::XMFLOAT3 scale, std::string sMatName)
{
	if (GameObject::Init(sName, position, rotation, scale) == false)
	{
		return false;
	}

	m_pMaterial = MaterialManager::GetInstance()->GetMaterial(sMatName);

	if (m_pMaterial == nullptr)
	{
		return false;
	}

	ID3D12Device4* pDevice = App::GetApp()->GetDevice();
	ID3D12GraphicsCommandList4* pCommandList = App::GetApp()->GetGraphicsCommandList();

	std::array<Vertex, 8> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f) })
	};

	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};

	Vertex vertex;
	XMFLOAT3 triNormal;

	std::uint16_t index0;
	std::uint16_t index1;
	std::uint16_t index2;

	//Calculate normal for all faces and add all together
	for (UINT i = 0; i < 12; ++i)
	{
		index0 = indices[i * 3];
		index1 = indices[i * 3 + 1];
		index2 = indices[i * 3 + 2];

		vertex = vertices[index0];

		XMStoreFloat3(&triNormal, XMVector3Normalize(XMVector3Cross(XMVector3Normalize(XMLoadFloat3(&vertices[index1].position) - XMLoadFloat3(&vertex.position)), XMLoadFloat3(&vertices[index2].position) - XMLoadFloat3(&vertex.position))));

		XMStoreFloat3(&vertices[index0].normal, XMLoadFloat3(&vertices[index0].normal) + XMLoadFloat3(&triNormal));
		XMStoreFloat3(&vertices[index1].normal, XMLoadFloat3(&vertices[index1].normal) + XMLoadFloat3(&triNormal));
		XMStoreFloat3(&vertices[index2].normal, XMLoadFloat3(&vertices[index2].normal) + XMLoadFloat3(&triNormal));
	}

	//Normalize all the vectors to average the normal
	for (UINT i = 0; i < 8; ++i)
	{
		XMStoreFloat3(&vertices[i].normal, XMVector3Normalize(XMLoadFloat3(&vertices[i].normal)));
	}

	const UINT uiVertexBufferByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT uiIndexBufferByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	m_pMeshGeometry = new MeshGeometry();

	HRESULT hr = D3DCreateBlob(uiVertexBufferByteSize, &m_pMeshGeometry->m_pVertexBufferCPU);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to Create blob for the vertex buffer (CPU)!");

		return false;
	}

	CopyMemory(m_pMeshGeometry->m_pVertexBufferCPU->GetBufferPointer(), vertices.data(), uiVertexBufferByteSize);

	hr = D3DCreateBlob(uiIndexBufferByteSize, &m_pMeshGeometry->m_pIndexBufferCPU);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to Create blob for the index buffer (CPU)!");

		return false;
	}

	CopyMemory(m_pMeshGeometry->m_pIndexBufferCPU->GetBufferPointer(), indices.data(), uiIndexBufferByteSize);

	m_pMeshGeometry->m_pVertexBufferGPU = DirectXHelper::CreateDefaultBuffer(pDevice,
																			pCommandList,
																			vertices.data(),
																			uiVertexBufferByteSize,
																			m_pMeshGeometry->m_pVertexBufferUploader);

	m_pMeshGeometry->m_pIndexBufferGPU = DirectXHelper::CreateDefaultBuffer(pDevice,
																			pCommandList,
																			indices.data(),
																			uiIndexBufferByteSize,
																			m_pMeshGeometry->m_pIndexBufferUploader);

	m_pMeshGeometry->m_uiVertexByteStride = sizeof(Vertex);
	m_pMeshGeometry->m_uiVertexByteSize = uiVertexBufferByteSize;
	m_pMeshGeometry->m_IndexFormat = DXGI_FORMAT_R16_UINT;
	m_pMeshGeometry->m_uiIndexByteSize = uiIndexBufferByteSize;

	SubMeshGeometry subMesh;
	subMesh.m_uiIndexCount = (UINT)indices.size();
	subMesh.m_uiStartIndexLocation = 0;
	subMesh.m_uiStartVertexLocation = 0;

	m_pMeshGeometry->m_SubMeshes["box"] = subMesh;

	m_eType = GameObjectType::VISIBLE;

	return true;
}

void VisibleGameObject::Draw()
{
	ID3D12GraphicsCommandList4* pCommandList = App::GetApp()->GetGraphicsCommandList();

	pCommandList->IASetVertexBuffers(0, 1, &m_pMeshGeometry->GetVertexBufferView());
	pCommandList->IASetIndexBuffer(&m_pMeshGeometry->GetIndexBufferView());

	for (std::unordered_map<std::string, SubMeshGeometry>::iterator it = m_pMeshGeometry->m_SubMeshes.begin(); it != m_pMeshGeometry->m_SubMeshes.end(); ++it)
	{
		pCommandList->DrawIndexedInstanced(it->second.m_uiIndexCount, 1, it->second.m_uiStartIndexLocation, 0, it->second.m_uiStartVertexLocation);
	}
}

Material* VisibleGameObject::GetMaterial()
{
	return m_pMaterial;
}
