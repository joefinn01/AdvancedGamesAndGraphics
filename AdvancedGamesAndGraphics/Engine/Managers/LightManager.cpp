#include "LightManager.h"
#include "Engine/DirectX/Light.h"
#include "Engine/Helpers/DebugHelper.h"

Tag tag = L"LightManager";

LightManager::LightManager()
{
	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		m_Lights[i] = new Light();
	}
}

Light* LightManager::GetLight(int iIndex)
{
	return m_Lights[iIndex];
}

bool LightManager::AddLight(Light* pLight)
{
	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		if (m_Lights[i]->Enabled == false)
		{
			delete m_Lights[i];
			m_Lights[i] = pLight;

			return true;
		}
	}

	LOG_ERROR(tag, L"Tried to add a light but all lights are in use!");

	return false;
}

void LightManager::ToggleLight(int iIndex)
{
	m_Lights[iIndex]->Enabled = 1 - m_Lights[iIndex]->Enabled;
}

void LightManager::SetLightState(int iIndex, bool bEnabled)
{
	m_Lights[iIndex]->Enabled = (int)bEnabled;
}
