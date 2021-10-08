#include "ObjectManager.h"
#include "Engine/GameObjects/GameObject.h"
#include "Engine/GameObjects/VisibleGameObject.h"
#include "Engine/Helpers/DebugHelper.h"
#include "Engine/Cameras/Camera.h"
#include "Engine/Apps/App.h"

Tag tag = L"ObjectManager";

bool ObjectManager::AddGameObject(GameObject* pGameObject)
{
	if (m_GameObjects.count(pGameObject->GetName()) == 1)
	{
		LOG_ERROR(tag, L"Tried to add an object with the name %s but an object with that name already exists!", pGameObject->GetName());

		return false;
	}

	m_GameObjects[pGameObject->GetName()] = pGameObject;

	return true;
}

bool ObjectManager::RemoveGameObject(const std::string& ksName)
{
	if (m_GameObjects.count(ksName) == 0)
	{
		LOG_ERROR(tag, L"Tried to remove an object with the name %s but no object with that name exists!", ksName);

		return false;
	}

	m_GameObjects.erase(ksName);

	return true;
}

bool ObjectManager::RemoveGameObject(GameObject* pGameObject)
{
	return RemoveGameObject(pGameObject->GetName());
}

GameObject* ObjectManager::GetGameObject(const std::string& ksName)
{
	return m_GameObjects.at(ksName);
}

std::unordered_map<std::string, GameObject*>* ObjectManager::GetGameObjects()
{
	return &m_GameObjects;
}

int ObjectManager::GetNumGameObjects()
{
	return (int)m_GameObjects.size();
}

bool ObjectManager::AddCamera(Camera* pCamera)
{
	if (m_Cameras.count(pCamera->GetName()) == 1)
	{
		LOG_ERROR(tag, L"Tried to add an object with the name %s but an object with that name already exists!", pCamera->GetName());

		return false;
	}

	m_Cameras[pCamera->GetName()] = pCamera;

	return true;
}

bool ObjectManager::RemoveCamera(const std::string& ksName)
{
	if (m_Cameras.count(ksName) == 0)
	{
		LOG_ERROR(tag, L"Tried to remove an object with the name %s but no object with that name exists!", ksName);

		return false;
	}

	m_Cameras.erase(ksName);

	return true;
}

bool ObjectManager::RemoveCamera(Camera* pCamera)
{
	return RemoveCamera(pCamera->GetName());
}

Camera* ObjectManager::GetActiveCamera() const
{
	return m_pActiveCamera;
}

Camera* ObjectManager::GetCamera(const std::string& ksName) const
{
	return m_Cameras.at(ksName);
}

void ObjectManager::SetActiveCamera(Camera* pCamera)
{
	m_pActiveCamera = pCamera;
}
