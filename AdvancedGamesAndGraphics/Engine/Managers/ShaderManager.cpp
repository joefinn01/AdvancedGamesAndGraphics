#include "ShaderManager.h"

#include "Engine/Helpers/DebugHelper.h"

using namespace Microsoft::WRL;

bool ShaderManager::RemoveShader(std::string sName)
{
	if (m_Shaders.count(sName) == 0)
	{
		LOG_WARNING(tag, L"Tried to remove a shader called %s but that shader doesn't exist!", sName);

		return false;
	}

	m_Shaders.erase(sName);

	return true;
}
