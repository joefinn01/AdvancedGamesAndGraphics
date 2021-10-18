#pragma once
#include "GameObject.h"

struct MeshGeometry;
struct Material;
struct D3DTextureData;
struct Vertex;

class VisibleGameObject : public GameObject
{
public:
	virtual bool Init(std::string sName, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation, DirectX::XMFLOAT3 scale, std::string sMatName, std::string sTexName);

	virtual void Draw();

	Material* GetMaterial();

	D3DTextureData* GetTexture();

protected:
	void CalculateTangentBinormal(Vertex v0, Vertex v1, Vertex v2, DirectX::XMFLOAT3& normal, DirectX::XMFLOAT3& tangent, DirectX::XMFLOAT3& binormal);

	MeshGeometry* m_pMeshGeometry = nullptr;

	Material* m_pMaterial = nullptr;

	D3DTextureData* m_pTexture = nullptr;
private:

};

