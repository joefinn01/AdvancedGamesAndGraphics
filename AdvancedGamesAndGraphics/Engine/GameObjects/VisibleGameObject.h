#pragma once
#include "GameObject.h"

struct MeshGeometry;
struct Material;
struct D3DTextureData;

class VisibleGameObject : public GameObject
{
public:
	virtual bool Init(std::string sName, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation, DirectX::XMFLOAT3 scale, std::string sMatName, std::string sTexName);

	virtual void Draw();

	Material* GetMaterial();

	D3DTextureData* GetTexture();

protected:
	MeshGeometry* m_pMeshGeometry = nullptr;

	Material* m_pMaterial = nullptr;

	D3DTextureData* m_pTexture = nullptr;
private:

};

