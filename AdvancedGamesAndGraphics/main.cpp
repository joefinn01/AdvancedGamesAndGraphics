#include "Engine/Apps/App.h"
#include "Engine/Helpers/DebugHelper.h"

#include <d3d12.h>
#include <windows.h>
#include <DirectXMath.h>

using namespace DirectX;

struct Vertex 
{
	Vertex(float x, float y, float z, float u, float v) : pos(x, y, z), texCoord(u, v) {}
	XMFLOAT3 pos;
	XMFLOAT2 texCoord;
};

Tag tag = "Main";

App* pApp = nullptr;

int WINAPI WinMain(HINSTANCE hInstance,    //Main windows function
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)

{
	pApp = new App(hInstance);

	if (App::GetApp()->Init() == false)
	{
		LOG_ERROR(tag, "Failed to Init D3DApp!");

		return 0;
	}

	return App::GetApp()->Run();
}