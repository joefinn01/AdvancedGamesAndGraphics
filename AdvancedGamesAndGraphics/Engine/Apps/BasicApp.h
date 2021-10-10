#pragma once
#include "Engine\Apps\App.h"
#include "Engine/DirectX/UploadBuffer.h"
#include "Engine/DirectX/ConstantBuffers.h"
#include "Engine/DirectX/Material.h"

#include <DirectXMath.h>
#include <vector>

class Timer;

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
	void CreateMaterialsUploadBuffer();
	void CreateShadersAndUploadBuffers();
	void CreateInputDescriptions();

	bool CreateRootSignature();
	bool CreateDescriptorHeaps();
	bool CreatePSOs();

	void InitIMGUI();
	void CreateIMGUIWindow();

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState = nullptr;

	UploadBuffer<PerFrameCB>* m_pPerFrameCB = nullptr;
	UploadBuffer<MaterialCB>* m_pMaterialCB = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pIMGUIDescHeap = nullptr;

	std::vector<D3D12_INPUT_ELEMENT_DESC> m_VertexInputLayoutDesc;

	bool m_bShowDemoWindow = true;

private:

};

