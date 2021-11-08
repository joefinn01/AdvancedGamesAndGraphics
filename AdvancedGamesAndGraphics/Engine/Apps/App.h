#pragma once

#include "Engine/Structure/Singleton.h"
#include "Engine/Commons/Timer.h"

#include <wrl.h>
#include <Windows.h>
#include <DirectX\d3dx12.h>
#include <dxgi1_4.h>

#define GBUFFER_NUM 6

class Timer;

struct GBuffer
{
	GBuffer()
	{
		m_pAlbedo = nullptr;
		m_pNormal = nullptr;
		m_pTangent = nullptr;
		m_pDiffuse = nullptr;
		m_pSpecular = nullptr;
		m_pAmbient = nullptr;
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pAlbedo;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pNormal;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pTangent;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pDiffuse;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pSpecular;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pAmbient;
};

class App
{
public:
	App(HINSTANCE hInstance);
	~App();

	virtual bool Init();

#if PIX
	static std::wstring GetPixGpuCapturePath();
#endif

	virtual void Update(const Timer& kTimer);
	virtual void OnResize();

	virtual void Draw() = 0;

	int Run();

	virtual void Load() = 0;

	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	bool Get4xMSAAState() const;
	void Set4xMSAAState(bool bState);

	ID3D12GraphicsCommandList4* GetGraphicsCommandList();
	ID3D12Device* GetDevice();

	static App* GetApp();

protected:
	bool InitWindow();
	bool InitDirectX3D();

	void FlushCommandQueue();

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* pAdapter, bool bSaveSettings);
	void LogOutputInfo(IDXGIOutput* pOutput, DXGI_FORMAT format, bool bSaveSettings);

	ID3D12Resource* GetCurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

	static App* m_pApp;

	Timer m_Timer;

	bool m_bPaused = false;
	bool m_bMinimized = false;
	bool m_bMaximized = false;
	bool m_bResizing = false;

	HINSTANCE m_AppInstance = nullptr;

	Microsoft::WRL::ComPtr<IDXGIFactory4> m_pDXGIFactory = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Device> m_pDevice = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_pFence = nullptr;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_pSwapChain = nullptr;
	static const UINT s_kuiSwapChainBufferCount = 2;

	int m_iBackBufferIndex = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_pCommandQueue = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_pCommandAllocator = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_pGraphicsCommandList = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pRTVHeap = nullptr;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDSVHeap = nullptr;

	GBuffer m_GBuffer;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pSwapChainBuffer[s_kuiSwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pDepthStencilBuffer;

	bool m_b4xMSAAState = false;
	UINT m_uiMSAAQuality = 0;

	UINT m_uiRTVDescSize = 0;
	UINT m_uiDSVDescSize = 0;
	UINT m_uiCBVSRVDescSize = 0;

	UINT64 m_uiCurrentFence = 0;

	D3D12_VIEWPORT m_Viewport = D3D12_VIEWPORT();
	D3D12_RECT m_ScissorRect = D3D12_RECT();

	DXGI_FORMAT m_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT m_DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

#if _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> m_pDebug = nullptr;
#endif

private:

};

