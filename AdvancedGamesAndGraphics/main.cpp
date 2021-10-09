#include "Engine/Apps/App.h"
#include "Engine/Apps/BasicApp.h"
#include "Engine/Helpers/DebugHelper.h"

#include <windows.h>

using namespace DirectX;

struct Vertex 
{
	Vertex(float x, float y, float z, float u, float v) : pos(x, y, z), texCoord(u, v) {}
	XMFLOAT3 pos;
	XMFLOAT2 texCoord;
};

Tag tag = L"Main";

App* pApp = nullptr;

int WINAPI WinMain(HINSTANCE hInstance,    //Main windows function
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)

{
	pApp = new BasicApp(hInstance);

	if (App::GetApp()->Init() == false)
	{
		LOG_ERROR(tag, L"Failed to Init App!");

		return 0;
	}

	return App::GetApp()->Run();
}