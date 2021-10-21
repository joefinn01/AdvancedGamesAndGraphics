#pragma once
#include "GameObject.h"

#include <vector>

struct MeshGeometry;
struct Material;
struct D3DTextureData;
struct Vertex;

class VisibleGameObject : public GameObject
{
public:
	virtual bool Init(std::string sName, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation, DirectX::XMFLOAT3 scale, std::string sMatName, std::vector<std::string> sTexNames);

	virtual void Draw();

	Material* GetMaterial();

	std::vector<D3DTextureData*> GetTextures();

protected:
	void CalculateTangentBinormal(Vertex v0, Vertex v1, Vertex v2, DirectX::XMFLOAT3& normal, DirectX::XMFLOAT3& tangent, DirectX::XMFLOAT3& binormal);

	MeshGeometry* m_pMeshGeometry = nullptr;

	Material* m_pMaterial = nullptr;

	std::vector<D3DTextureData*> m_pTextures = std::vector<D3DTextureData*>();
private:

};

