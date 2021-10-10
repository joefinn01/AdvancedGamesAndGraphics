#include "MaterialManager.h"
#include "Engine/Helpers/DebugHelper.h"

bool MaterialManager::AddMaterial(Material* pMat)
{
	if (m_Materials.count(pMat->name) == 1)
	{
		LOG_ERROR(tag, L"Tried to add shader called %s but one with that name already exists!", ksName);

		return false;
	}

	m_Materials[pMat->name] = pMat;
}
