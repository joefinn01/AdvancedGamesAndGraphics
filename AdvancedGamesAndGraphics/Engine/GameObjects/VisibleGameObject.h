#pragma once
#include "GameObject.h"

struct MeshGeometry;

class VisibleGameObject : public GameObject
{
public:
	virtual bool Init(std::string sName, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation, DirectX::XMFLOAT3 scale) override;

	virtual void Draw();

protected:
	MeshGeometry* m_pMeshGeometry = nullptr;

private:

};

