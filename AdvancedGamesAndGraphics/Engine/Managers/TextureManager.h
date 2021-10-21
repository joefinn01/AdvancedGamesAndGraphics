#pragma once
#include "Engine/Structure/Singleton.h"

#include <unordered_map>
#include <DirectX/d3dx12.h>
#include <wrl/client.h>

struct D3DTextureData
{
	Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap;
	int HeapIndex;
};

class TextureManager : public Singleton<TextureManager>
{
public:
	bool AddTexture(std::string sName, std::wstring wsFilename);
	bool AddTexture(std::string sName, D3DTextureData* pTexData);

	bool RemoveTexture(std::string sName);

	D3DTextureData* GetTexture(std::string sName);

	std::unordered_map<std::string, D3DTextureData*>* GetTextures();

protected:

private:
	std::unordered_map<std::string, D3DTextureData*> m_Textures;
};

