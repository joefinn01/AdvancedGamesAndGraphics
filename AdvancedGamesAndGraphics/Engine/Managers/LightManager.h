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

protected:

private:
	Light* m_Lights[MAX_LIGHTS];
};

