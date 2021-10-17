#include "VisibleGameObject.h"
#include "Engine/Apps/App.h"
#include "Engine/DirectX/Vertices.h"
#include "Engine/DirectX/MeshGeometry.h"
#include "Engine/Helpers/DebugHelper.h"
#include "Engine/Helpers/DirectXHelper.h"
#include "Engine/Managers/MaterialManager.h"
#include "Engine/Managers/TextureManager.h"

#include <DirectX\d3dx12.h>
#include <DirectXColors.h>
#include <array>
#include <d3dcompiler.h>

using namespace DirectX;

Tag tag = L"VisibleGameObject";

bool VisibleGameObject::Init(std::string sName, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation, DirectX::XMFLOAT3 scale, std::string sMatName, std::string sTexName)
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

	m_pTexture = TextureManager::GetInstance()->GetTexture(sTexName);

	if (m_pTexture == nullptr)
	{
		return false;
	}

	ID3D12Device* pDevice = App::GetApp()->GetDevice();
	ID3D12GraphicsCommandList4* pCommandList = App::GetApp()->GetGraphicsCommandList();

	//std::array<Vertex, 8> vertices =
	//{
	//	Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f) }),
	//	Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f) }),
	//	Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f) }),
	//	Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f) }),
	//	Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f) }),
	//	Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f) }),
	//	Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f) }),
	//	Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f) })
	//};

	std::array<Vertex, 24> vertices =
	{
		//Front face
		Vertex(XMFLOAT3(-1, -1, -1), XMFLOAT3(0, 0, -1), XMFLOAT2(0, 1)),
		Vertex(XMFLOAT3(-1, 1, -1), XMFLOAT3(0, 0, -1), XMFLOAT2(0, 0)),
		Vertex(XMFLOAT3(1, 1, -1), XMFLOAT3(0, 0, -1), XMFLOAT2(1, 0)),
		Vertex(XMFLOAT3(1, -1, -1), XMFLOAT3(0, 0, -1), XMFLOAT2(1, 1)),

		//Back face
		Vertex(XMFLOAT3(-1, -1, 1), XMFLOAT3(0, 0, 1), XMFLOAT2(1, 1)),
		Vertex(XMFLOAT3(1, -1, 1), XMFLOAT3(0, 0, 1), XMFLOAT2(0, 1)),
		Vertex(XMFLOAT3(1, 1, 1), XMFLOAT3(0, 0, 1), XMFLOAT2(0, 0)),
		Vertex(XMFLOAT3(-1, 1, 1), XMFLOAT3(0, 0, 1), XMFLOAT2(1, 0)),

		//Top face
		Vertex(XMFLOAT3(-1, 1, -1), XMFLOAT3(0, 1, 0), XMFLOAT2(0, 1)),
		Vertex(XMFLOAT3(-1, 1, 1), XMFLOAT3(0, 1, 0), XMFLOAT2(0, 0)),
		Vertex(XMFLOAT3(1, 1, 1), XMFLOAT3(0, 1, 0), XMFLOAT2(1, 0)),
		Vertex(XMFLOAT3(1, 1, -1), XMFLOAT3(0, 1, 0), XMFLOAT2(1, 1)),

		//Bottom face
		Vertex(XMFLOAT3(-1, -1, -1), XMFLOAT3(0, -1, 0), XMFLOAT2(1, 1)),
		Vertex(XMFLOAT3(1, -1, -1), XMFLOAT3(0, -1, 0), XMFLOAT2(0, 1)),
		Vertex(XMFLOAT3(1, -1, 1), XMFLOAT3(0, -1, 0), XMFLOAT2(0, 0)),
		Vertex(XMFLOAT3(-1, -1, 1), XMFLOAT3(0, -1, 0), XMFLOAT2(1, 0)),

		//Left face
		Vertex(XMFLOAT3(-1, -1, 1), XMFLOAT3(-1, 0, 0), XMFLOAT2(0, 1)),
		Vertex(XMFLOAT3(-1, 1, 1), XMFLOAT3(-1, 0, 0), XMFLOAT2(0, 0)),
		Vertex(XMFLOAT3(-1, 1, -1), XMFLOAT3(-1, 0, 0), XMFLOAT2(1, 0)),
		Vertex(XMFLOAT3(-1, -1, -1), XMFLOAT3(-1, 0, 0), XMFLOAT2(1, 1)),

		//Right face
		Vertex(XMFLOAT3(1, -1, -1), XMFLOAT3(1, 0, 0), XMFLOAT2(0, 1)),
		Vertex(XMFLOAT3(1, 1, -1), XMFLOAT3(1, 0, 0), XMFLOAT2(0, 0)),
		Vertex(XMFLOAT3(1, 1, 1), XMFLOAT3(1, 0, 0), XMFLOAT2(1, 0)),
		Vertex(XMFLOAT3(1, -1, 1), XMFLOAT3(1, 0, 0), XMFLOAT2(1, 1)),
	};

	std::array<std::uint16_t, 36> indices =
	{
		//Front face
		0, 1, 2,
		0, 2, 3,

		//Back face
		4, 5, 6,
		4, 6, 7,

		//Top face
		8, 9, 10,
		8, 10, 11,

		//Bottom face
		12, 13, 14,
		12, 14, 15,

		//Left face
		16, 17, 18,
		16, 18, 19,

		//Right face
		20, 21, 22,
		20, 22, 23
	};

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

D3DTextureData* VisibleGameObject::GetTexture()
{
	return m_pTexture;
}
