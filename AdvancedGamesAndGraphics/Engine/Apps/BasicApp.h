#pragma once
#include "Engine\Apps\App.h"
#include "Engine/DirectX/UploadBuffer.h"
#include "Engine/DirectX/ConstantBuffers.h"
#include "Engine/DirectX/Material.h"
#include "Engine/Managers/InputManager.h"

#include <DirectXMath.h>
#include <vector>
#include <array>

class Timer;

struct Light;

struct PSODesc
{
	std::string VSName;
	std::string PSName;

	bool operator== (const PSODesc& rhs) const
	{
		return VSName == rhs.VSName && PSName == rhs.PSName;
	}
};

namespace std
{
	template<>
	struct hash<PSODesc>
	{
		std::size_t operator()(const PSODesc& psoDesc) const
		{
			return ((hash<string>()(psoDesc.VSName)
				^ (hash<string>()(psoDesc.PSName) << 1)) >> 1);
		}
	};

	template<>
	struct equal_to<PSODesc>
	{
		bool operator()(const PSODesc& lhs, const PSODesc& rhs) const
		{
			return lhs == rhs;
		}
	};
}

class BasicApp : public App
{
public:
	BasicApp(HINSTANCE hInstance);

	virtual bool Init() override;

	virtual void Update(const Timer& kTimer) override;
	virtual void Draw() override;

	virtual void Load() override;

protected:
	void ExecuteCommandList();

	void CreateGameObjects();
	void CreateMaterials();
	void CreateTextures();
	void CreateMaterialsUploadBuffer();
	void CreateShadersAndUploadBuffers();
	void CreateInputDescriptions();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

	bool CreateRootSignature();
	bool CreateDescriptorHeaps();
	void PopulateTextureHeap();
	
	bool CreatePSOs();
	bool CreateGBufferPSO();

	void InitIMGUI();
	void CreateIMGUIWindow();

	void PopulateGBuffer();
	void RenderToTexture();
	void PostProcessing();

	static void OnKeyDown(void* pObject, int iKeycode);
	static void OnKeyHeld(void* pObject, int iKeycode, const Timer& ktimer);

	InputObserver m_Observer;
	InputObserver m_MovementObserver;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;

	std::unordered_map<PSODesc, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_PipelineStates;
	ID3D12PipelineState* m_pPipelineState;

	UploadBuffer<PerFrameCB>* m_pPerFrameCB = nullptr;
	UploadBuffer<MaterialCB>* m_pMaterialCB = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pIMGUIDescHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pTextureDescHeap = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_VertexInputLayoutDesc;

	bool m_bShowDemoWindow = false;
	bool m_bRotateCube = false;

	Vertex* m_QuadVertices;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pScreenQuadVertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pScreenQuadVertexBufferUploader = nullptr;

	D3D12_VERTEX_BUFFER_VIEW m_ScreenQuadVBView;

private:

};

