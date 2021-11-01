#pragma once
#include "Engine/Structure/Singleton.h"

#define MAX_LIGHTS 16

struct Light;

class LightManager : public Singleton<LightManager>
{
public:
	LightManager();

	Light* GetLight(int iIndex);

	bool AddLight(Light* pLight);

	void ToggleLight(int iIndex);
	void SetLightState(int iIndex, bool bEnabled);

protected:

private:
	Light* m_Lights[MAX_LIGHTS];
};

