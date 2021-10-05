#pragma once
#include "Engine\Structure\Singleton.h"

#include <string>
#include <unordered_map>

class Timer;
class GameObject;
class Camera;

struct ID3D12GraphicsCommandList4;

class ObjectManager : public Singleton<ObjectManager>
{
public:
	//GameObject methods
	bool AddGameObject(GameObject* pGameObject);

	bool RemoveGameObject(const std::string& ksName);
	bool RemoveGameObject(GameObject* pGameObject);

	GameObject* GetGameObject(const std::string& ksName);

	//Camera methods
	bool AddCamera(Camera* pCamera);

	bool RemoveCamera(const std::string& ksName);
	bool RemoveCamera(Camera* pCamera);

	Camera* GetActiveCamera() const;
	void SetActiveCamera(Camera* pCamera);

	Camera* GetCamera(const std::string& ksName) const;

	void Update(const Timer& kTimer);

	void Draw();



protected:

private:
	std::unordered_map<std::string, GameObject*> m_GameObjects;
	std::unordered_map<std::string, Camera*> m_Cameras;

	Camera* m_pActiveCamera = nullptr;
};

