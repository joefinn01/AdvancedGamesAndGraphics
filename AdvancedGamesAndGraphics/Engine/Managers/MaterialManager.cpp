#include "MaterialManager.h"
#include "Engine/Helpers/DebugHelper.h"

Tag tag = L"MaterialManager";

bool MaterialManager::AddMaterial(Material* pMat)
{
	if (m_Materials.count(pMat->name) == 1)
	{
		LOG_ERROR(tag, L"Tried to add material called %s but one with that name already exists!", pMat->name);

		return false;
	}

	m_Materials[pMat->name] = pMat;
}

Material* MaterialManager::GetMaterial(std::string sName)
{
	if (m_Materials.count(sName) == 0)
	{
		LOG_ERROR(tag, L"Tried to get material called %s but one with that name doesn't exist!", sName);

		return false;
	}

	return m_Materials[sName];
}

std::unordered_map<std::string, Material*>* MaterialManager::GetMaterials()
{
	return &m_Materials;
}
