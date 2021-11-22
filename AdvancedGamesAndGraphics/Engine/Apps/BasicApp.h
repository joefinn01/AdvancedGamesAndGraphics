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
struct Vertex;

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

	virtual void OnResize() override;

protected:
	void ExecuteCommandList();

	void CreateGameObjects();
	void CreateMaterials();
	void CreateTextures();
	void CreateMaterialsUploadBuffer();
	void CreateShadersAndUploadBuffers();
	void CreateInputDescriptions();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

	bool CreateRootSignatures();
	bool CreateGBufferRootSignature();
	bool CreateLightPassRootSignature();
	bool CreatePostProcessingRootSignature();

	bool CreateDescriptorHeaps();
	void PopulateTextureHeap();
	
	bool CreatePSOs();
	bool CreateGBufferPSO();
	bool CreateLightPassPSO();
	bool CreatePostProcessingPSO();

	void InitIMGUI();
	void CreateIMGUIWindow();

	void PopulateGBuffer();
	void DoLightPass();
	void DoPostProcessing();

	void UpdatePostProcessingCB();

	static void OnKeyDown(void* pObject, int iKeycode);
	static void OnKeyHeld(void* pObject, int iKeycode, const Timer& ktimer);

	InputObserver m_Observer;
	InputObserver m_MovementObserver;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pGBufferSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pLightSignature;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pPostProcessingSignature;

	std::unordered_map<PSODesc, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_PipelineStates;
	ID3D12PipelineState* m_pPipelineState;

	UploadBuffer<LightPassPerFrameCB>* m_pLightPassPerFrameCB = nullptr;
	UploadBuffer<GBufferPerFrameCB>* m_pGBufferPerFrameCB = nullptr;
	UploadBuffer<PostProcessingPerFrameCB>* m_pPostProcessingPerFrameCB = nullptr;
	UploadBuffer<MaterialCB>* m_pMaterialCB = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pIMGUIDescHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pTextureDescHeap = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_GBufferVertexInputLayoutDesc;
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_ScreenQuadVertexInputLayoutDesc;

	bool m_bShowDemoWindow = false;
	bool m_bRotateCube = false;
	bool m_bEnableBoxBlur = false;

	int m_iBoxBlurLevel = 6;

	Vertex* m_QuadVertices;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pScreenQuadVertexBufferGPU = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pScreenQuadVertexBufferUploader = nullptr;

	D3D12_VERTEX_BUFFER_VIEW m_ScreenQuadVBView;

	std::string m_sCurrentGBufferPSName = "PS_GBuffer_Nothing";
	std::string m_sCurrentLightPassPSName = "PS_LightPass_Nothing";

private:

};

