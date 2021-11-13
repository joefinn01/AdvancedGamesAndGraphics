#include "TextureManager.h"
#include "Engine/Helpers/DebugHelper.h"
#include "Engine/DDSTextureLoader.h"
#include "Engine/Apps/App.h"

Tag sTag = L"TextureManager";

bool TextureManager::AddTexture(std::string sName, std::wstring wsFilename)
{
	if (m_Textures.count(sName) != 0)
	{
		LOG_ERROR(sTag, L"Tried to add a texture called %s but one with that name already exists!", sName);

		return false;
	}

	D3DTextureData* pTexData = new D3DTextureData();

	HRESULT hr = DirectX::CreateDDSTextureFromFile12(App::GetApp()->GetDevice(), App::GetApp()->GetGraphicsCommandList(), wsFilename.c_str(), pTexData->Resource, pTexData->UploadHeap);

	if (FAILED(hr))
	{
		LOG_ERROR(sTag, L"Failed to create texture with name %ls!", wsFilename);

		delete pTexData;

		return false;
	}

	m_Textures[sName] = pTexData;

	return true;
}

bool TextureManager::AddTexture(std::string sName, D3DTextureData* pTexData)
{
	if (m_Textures.count(sName) != 0)
	{
		LOG_ERROR(sTag, L"Tried to add a texture called %s but one with that name already exists!", sName);

		return false;
	}

	m_Textures[sName] = pTexData;

	return true;
}

bool TextureManager::RemoveTexture(std::string sName)
{
	if (m_Textures.count(sName) != 1)
	{
		LOG_ERROR(sTag, L"Tried to remove a texture called %s but one with that name doesn't exist!", sName);

		return false;
	}

	delete m_Textures[sName];
	m_Textures[sName] = nullptr;

	return true;
}

D3DTextureData* TextureManager::GetTexture(std::string sName)
{
	if (m_Textures.count(sName) != 1)
	{
		LOG_ERROR(sTag, L"Tried to get a texture called %s but one with that name doesn't exist!", sName);

		return nullptr;
	}

	return m_Textures[sName];
}

std::unordered_map<std::string, D3DTextureData*>* TextureManager::GetTextures()
{
	return &m_Textures;
}
