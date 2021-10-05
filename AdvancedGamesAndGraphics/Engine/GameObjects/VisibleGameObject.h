#pragma once
#include "GameObject.h"

struct MeshGeometry;

class VisibleGameObject : public GameObject
{
public:
	virtual bool Init(std::string sName, DirectX::XMFLOAT4 position, DirectX::XMFLOAT4 rotationQuat, DirectX::XMFLOAT4 scale) override;

	virtual void Draw();

protected:
	MeshGeometry* m_pMeshGeometry = nullptr;

private:

};

