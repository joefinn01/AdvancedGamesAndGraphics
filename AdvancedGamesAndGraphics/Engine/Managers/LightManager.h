#pragma once
#include "Engine/Structure/Singleton.h"
#include "Engine/DirectX/UploadBuffer.h"

#include <unordered_map>

#define MAX_LIGHTS 16

struct Light;
struct LightCB;

class LightManager : public Singleton<LightManager>
{
public:
	LightManager();

	Light* GetLight(std::string sName);

	bool AddLight(std::string sName, Light* pLight);
	bool RemoveLight(std::string sName);

	void ToggleLight(std::string sName);
	void SetLightState(std::string sName, bool bEnabled);

	UploadBuffer<LightCB>* GetUploadBuffer();

	std::unordered_map<std::string, Light*>* GetLights();

	void UpdateUploadBuffer(std::string sName);

protected:

private:
	void ResizeUploadBuffer();

	std::unordered_map<std::string, Light*> m_Lights;

	UploadBuffer<LightCB>* m_pUploadBuffer;
};

