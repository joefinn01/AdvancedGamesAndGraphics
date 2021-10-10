#pragma once
#include "GameObject.h"

struct MeshGeometry;
struct Material;

class VisibleGameObject : public GameObject
{
public:
	virtual bool Init(std::string sName, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation, DirectX::XMFLOAT3 scale, std::string sMatName);

	virtual void Draw();

	Material* GetMaterial();

protected:
	MeshGeometry* m_pMeshGeometry = nullptr;

	Material* m_pMaterial = nullptr;
private:

};

