#include "LightManager.h"
#include "Engine/DirectX/ConstantBuffers.h"
#include "Engine/DirectX/Light.h"
#include "Engine/Helpers/DebugHelper.h"
#include "Engine/Apps/App.h"

Tag tag = L"LightManager";

LightManager::LightManager()
{
	m_pUploadBuffer = new UploadBuffer<LightCB>(App::GetApp()->GetDevice(), 0, true);
}

Light* LightManager::GetLight(std::string sName)
{
	return m_Lights[sName];
}

bool LightManager::AddLight(std::string sName, Light* pLight)
{
	if (m_Lights.count(sName) == 1)
	{
		LOG_ERROR(tag, L"Tried to add light called %s but one with that name already exists!", sName);

		return false;
	}

	m_Lights[sName] = pLight;

	ResizeUploadBuffer();

	return true;
}

bool LightManager::RemoveLight(std::string sName)
{
	if (m_Lights.count(sName) == 0)
	{
		LOG_ERROR(tag, L"Tried to remove light called %s but one with that name doesn't exist!", sName);

		return false;
	}

	m_Lights.erase(sName);

	ResizeUploadBuffer();

	return true;
}

void LightManager::ToggleLight(std::string sName)
{
	if (m_Lights.count(sName) == 0)
	{
		LOG_ERROR(tag, L"Tried to toggle light called %s but none with that name exist!", sName);

		return;
	}

	m_Lights[sName]->Enabled = 1 - m_Lights[sName]->Enabled;
}

void LightManager::SetLightState(std::string sName, bool bEnabled)
{
	if (m_Lights.count(sName) == 0)
	{
		LOG_ERROR(tag, L"Tried to set light called %s state but none with that name exist!", sName);

		return;
	}

	m_Lights[sName]->Enabled = (int)bEnabled;
}

UploadBuffer<LightCB>* LightManager::GetUploadBuffer()
{
	return m_pUploadBuffer;
}

std::unordered_map<std::string, Light*>* LightManager::GetLights()
{
	return &m_Lights;
}

void LightManager::ResizeUploadBuffer()
{
	delete m_pUploadBuffer;

	m_pUploadBuffer = new UploadBuffer<LightCB>(App::GetApp()->GetDevice(), m_Lights.size(), true);

	LightCB lightCB;

	int iCount = 0;

	for (std::unordered_map<std::string, Light*>::iterator it = m_Lights.begin(); it != m_Lights.end(); ++it)
	{
		it->second->Index = iCount;

		lightCB = it->second->lightCB;

		m_pUploadBuffer->CopyData(iCount, lightCB);

		++iCount;
	}
}
