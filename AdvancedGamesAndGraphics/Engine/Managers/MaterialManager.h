#pragma once
#include "Engine\Structure\Singleton.h"
#include "Engine/DirectX/Material.h"

#include  <unordered_map>

class MaterialManager : public Singleton<MaterialManager>
{
public:
	bool AddMaterial(Material* pMat);

protected:

private:
	std::unordered_map<std::string, Material*> m_Materials;
};

