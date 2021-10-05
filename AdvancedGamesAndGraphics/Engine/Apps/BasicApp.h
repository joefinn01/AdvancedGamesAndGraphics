#pragma once
#include "Engine\Apps\App.h"
#include "Engine/DirectX/UploadBuffer.h"

#include <DirectXMath.h>

struct PerFrameCB
{
	DirectX::XMMATRIX ViewProjection;
};

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
	void ResetCommmandList();
	void ExecuteCommandList();


	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState = nullptr;

	UploadBuffer<PerFrameCB>* m_pPerFrameCB;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pConstDescHeap = nullptr;

private:

};

