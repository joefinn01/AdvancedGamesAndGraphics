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

	XMFLOAT3 tangent, binormal, normal;

	for (int i = 0; i < vertices.size() / 3; ++i)
	{
		CalculateTangentBinormal(vertices[i], vertices[i + 1], vertices[i + 2], normal, tangent, binormal);

		// Store the normal, tangent, and binormal for this face back in the model structure.
		//vertices[i].Normal.x = normal.x;
		//vertices[i].Normal.y = normal.y;
		//vertices[i].Normal.z = normal.z;
		vertices[i].Tangent.x = tangent.x;
		vertices[i].Tangent.y = tangent.y;
		vertices[i].Tangent.z = tangent.z;
		vertices[i].BiTangent.x = binormal.x;
		vertices[i].BiTangent.y = binormal.y;
		vertices[i].BiTangent.z = binormal.z;

		//vertices[i + 1].Normal.x = normal.x;
		//vertices[i + 1].Normal.y = normal.y;
		//vertices[i + 1].Normal.z = normal.z;
		vertices[i + 1].Tangent.x = tangent.x;
		vertices[i + 1].Tangent.y = tangent.y;
		vertices[i + 1].Tangent.z = tangent.z;
		vertices[i + 1].BiTangent.x = binormal.x;
		vertices[i + 1].BiTangent.y = binormal.y;
		vertices[i + 1].BiTangent.z = binormal.z;

		//vertices[i + 2].Normal.x = normal.x;
		//vertices[i + 2].Normal.y = normal.y;
		//vertices[i + 2].Normal.z = normal.z;
		vertices[i + 2].Tangent.x = tangent.x;
		vertices[i + 2].Tangent.y = tangent.y;
		vertices[i + 2].Tangent.z = tangent.z;
		vertices[i + 2].BiTangent.x = binormal.x;
		vertices[i + 2].BiTangent.y = binormal.y;
		vertices[i + 2].BiTangent.z = binormal.z;
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

D3DTextureData* VisibleGameObject::GetTexture()
{
	return m_pTexture;
}

void VisibleGameObject::CalculateTangentBinormal(Vertex v0, Vertex v1, Vertex v2, DirectX::XMFLOAT3& normal, DirectX::XMFLOAT3& tangent, DirectX::XMFLOAT3& binormal)
{
	XMVECTOR vv0 = XMLoadFloat3(&v0.Position);
	XMVECTOR vv1 = XMLoadFloat3(&v1.Position);
	XMVECTOR vv2 = XMLoadFloat3(&v2.Position);

	XMVECTOR P = vv1 - vv0;
	XMVECTOR Q = vv2 - vv0;

	// 2. CALCULATE THE TANGENT from texture space

	float s1 = v1.TexCoords.x - v0.TexCoords.x;
	float t1 = v1.TexCoords.y - v0.TexCoords.y;
	float s2 = v2.TexCoords.x - v0.TexCoords.x;
	float t2 = v2.TexCoords.y - v0.TexCoords.y;

	float tmp = 0.0f;
	if (fabsf(s1 * t2 - s2 * t1) <= 0.0001f)
	{
		tmp = 1.0f;
	}
	else
	{
		tmp = 1.0f / (s1 * t2 - s2 * t1);
	}

	XMFLOAT3 PF3, QF3;
	XMStoreFloat3(&PF3, P);
	XMStoreFloat3(&QF3, Q);

	tangent.x = (t2 * PF3.x - t1 * QF3.x);
	tangent.y = (t2 * PF3.y - t1 * QF3.y);
	tangent.z = (t2 * PF3.z - t1 * QF3.z);

	tangent.x = tangent.x * tmp;
	tangent.y = tangent.y * tmp;
	tangent.z = tangent.z * tmp;

	XMVECTOR vn = XMLoadFloat3(&normal);
	XMVECTOR vt = XMLoadFloat3(&tangent);

	// 3. CALCULATE THE BINORMAL
	// left hand system b = t cross n (rh would be b = n cross t)
	XMVECTOR vb = XMVector3Cross(vt, vn);

	vn = XMVector3Normalize(vn);
	vt = XMVector3Normalize(vt);
	vb = XMVector3Normalize(vb);

	XMStoreFloat3(&normal, vn);
	XMStoreFloat3(&tangent, vt);
	XMStoreFloat3(&binormal, vb);

	return;
}
