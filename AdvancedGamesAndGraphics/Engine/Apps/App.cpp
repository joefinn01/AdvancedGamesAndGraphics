#include "App.h"
#include "Engine/Commons/Timer.h"
#include "Engine/Helpers/DebugHelper.h"
#include "Engine/Managers/WindowManager.h"

#include <DirectX/d3dx12.h>
#include <windowsx.h>
#include <vector>

Tag tag = "App";

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return App::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

App::App(HINSTANCE hInstance)
{
	if (m_pApp != nullptr)
	{
		LOG_ERROR(tag, "Tried to create new app when one already exists!");

		return;
	}

	m_AppInstance = hInstance;

	m_pApp = this;
}

App::~App()
{
}

bool App::Init()
{
	UINT uiDXGIFactoryFlags = 0;
	HRESULT hr;

#if _DEBUG
	//Create debug controller
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(m_pDebug.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, "Failed to create Debug controller!");

		uiDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}

	m_pDebug->EnableDebugLayer();
#endif

	//Create the DXGI factory and output all information related to adapters.
	hr = CreateDXGIFactory2(uiDXGIFactoryFlags, IID_PPV_ARGS(m_pDXGIFactory.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, "Failed to create DXGIFactory!");

		return false;
	}

	//Log adapters before initialising anything else so we can get the correct window dimensions and refresh rate
	LogAdapters();

	if (!InitWindow())
	{
		return false;
	}

	if (!InitDirectX3D())
	{
		return false;
	}

	OnResize();

	Load();

	return true;
}

void App::OnResize()
{
	FlushCommandQueue();

	HRESULT hr = m_pGraphicsCommandList->Reset(m_pCommandAllocator.Get(), nullptr);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, "Failed to reset the graphics command list!");

		return;
	}

	// Release the previous resources we will be recreating.
	for (int i = 0; i < s_kuiSwapChainBufferCount; ++i)
	{
		m_pSwapChainBuffer[i].Reset();
	}

	m_pDepthStencilBuffer.Reset();

	// Resize the swap chain.
	hr = m_pSwapChain->ResizeBuffers(s_kuiSwapChainBufferCount, WindowManager::GetInstance()->GetWindowWidth(), WindowManager::GetInstance()->GetWindowHeight(), m_BackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

	m_iBackBufferIndex = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < s_kuiSwapChainBufferCount; i++)
	{
		hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pSwapChainBuffer[i]));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, "Failed to get swap chain buffer at index %u!", i);

			return;
		}

		m_pDevice->CreateRenderTargetView(m_pSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, m_uiRTVDescSize);
	}

	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = WindowManager::GetInstance()->GetWindowWidth();
	depthStencilDesc.Height = WindowManager::GetInstance()->GetWindowHeight();
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthStencilDesc.SampleDesc.Count = m_b4xMSAAState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m_b4xMSAAState ? (m_uiMSAAQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = m_DepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	hr = m_pDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(m_pDepthStencilBuffer.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, "Failed to Create a committed resource when resizing screen!");

		return;
	}

	// Create view for first mip map level
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = m_DepthStencilFormat;
	dsvDesc.Texture2D.MipSlice = 0;
	m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer.Get(), &dsvDesc, GetDepthStencilView());

	// Transition the resource from its initial state to be used as a depth buffer.
	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// Execute the resize commands.
	hr = m_pGraphicsCommandList->Close();

	if (FAILED(hr))
	{
		LOG_ERROR(tag, "Failed to close the command list!");

		return;
	}

	ID3D12CommandList* cmdsLists[] = { m_pGraphicsCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();

	// Update the viewport transform to cover the client area.
	m_Viewport.TopLeftX = 0;
	m_Viewport.TopLeftY = 0;
	m_Viewport.Width = static_cast<float>(WindowManager::GetInstance()->GetWindowWidth());
	m_Viewport.Height = static_cast<float>(WindowManager::GetInstance()->GetWindowHeight());
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;

	m_ScissorRect = { 0, 0, static_cast<long>(WindowManager::GetInstance()->GetWindowWidth()), static_cast<long>(WindowManager::GetInstance()->GetWindowHeight()) };
}

int App::Run()
{
	MSG msg = { 0 };

	m_Timer.Reset();

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			m_Timer.Tick();

			if (!m_bPaused)
			{
				Update(m_Timer);
				Draw();
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

LRESULT App::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			m_bPaused = true;
			m_Timer.Stop();
		}
		else
		{
			m_bPaused = false;
			m_Timer.Start();
		}

		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		WindowManager::GetInstance()->SetWindowDimensions(LOWORD(lParam), HIWORD(lParam));
		if (m_pDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				m_bPaused = true;
				m_bMinimized = true;
				m_bMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				m_bPaused = false;
				m_bMinimized = false;
				m_bMaximized = true;

				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{

				// Restoring from minimized state?
				if (m_bMinimized)
				{
					m_bPaused = false;
					m_bMinimized = false;

					OnResize();
				}

				// Restoring from maximized state?
				else if (m_bMaximized)
				{
					m_bPaused = false;
					m_bMaximized = false;

					OnResize();
				}
				else if (m_bResizing)
				{
					//don't do anything while resizing
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}

		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		m_bPaused = true;
		m_bResizing = true;
		m_Timer.Stop();

		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		m_bPaused = false;
		m_bResizing = false;
		m_Timer.Start();

		OnResize();

		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;

		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

		return 0;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

		return 0;

	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if ((int)wParam == VK_F2)
		{
			Set4xMSAAState(!m_b4xMSAAState);
		}

		OnKeyUp(wParam);

		return 0;

	case WM_KEYDOWN:
		OnKeyDown(wParam);

		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool App::Get4xMSAAState() const
{
	return m_b4xMSAAState;
}

void App::Set4xMSAAState(bool bState)
{
	m_b4xMSAAState = bState;
}

App* App::GetApp()
{
	return m_pApp;
}

bool App::InitWindow()
{
	WNDCLASS windowClass;
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = MainWndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = m_AppInstance;
	windowClass.hIcon = LoadIcon(0, IDI_APPLICATION);
	windowClass.hCursor = LoadCursor(0, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = L"Main Window";

	if (RegisterClass(&windowClass) == false)
	{
		MessageBox(0, L"RegisterClass Failed!", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, static_cast<long>(WindowManager::GetInstance()->GetWindowWidth()), static_cast<long>(WindowManager::GetInstance()->GetWindowHeight()) };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);

	int iWidth = R.right - R.left;
	int iHeight = R.bottom - R.top;

	HWND window = CreateWindow(L"Main Window",
		L"DirectX App",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		iWidth,
		iHeight,
		0,
		0,
		m_AppInstance,
		0);

	if (window == 0)
	{
		MessageBox(0, L"CreateWindow Failed!", 0, 0);
		return false;
	}

	ShowWindow(window, SW_SHOW);
	UpdateWindow(window);

	WindowManager::GetInstance()->SetHWND(window);

	return true;
}

bool App::InitDirectX3D()
{
	//Create device
	 HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(m_pDevice.GetAddressOf()));

	if (FAILED(hr))
	{
		Microsoft::WRL::ComPtr<IDXGIAdapter> pWarpAdapter;
		hr = m_pDXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(m_pDXGIFactory.GetAddressOf()));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, "Failed to enum warp adapters!");

			return false;
		}

		hr = D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_pDevice));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, "Failed to create device using hardware and WARP adaptor!");

			return false;
		}
	}

	//Create fence for synchronizing CPU and GPU
	hr = m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, "Failed to create fence!");

		return false;
	}

	//Cache different description sizes
	m_uiRTVDescSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_uiDSVDescSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_uiCBVSRVDescSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Check 4X MSAA quality support for our back buffer format.
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = m_BackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	hr = m_pDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, "Failed to get multisample quality level!");

		return false;
	}

	m_uiMSAAQuality = msQualityLevels.NumQualityLevels;

	//Create command objects

	//Create command list and queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	HRESULT hr = m_pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_pCommandQueue.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, "Failed to create the command queue!");

		return false;
	}

	hr = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pCommandAllocator.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, "Failed to create the command allocator!");

		return false;
	}

	hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(m_pGraphicsCommandList.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, "Failed to create the graphics command list!");

		return false;
	}

	//Close the command list so it can be reset
	m_pGraphicsCommandList->Close();

	//Create the swap chain
	m_pSwapChain.Reset();

	DXGI_SWAP_CHAIN_DESC1 chainDesc = {};
	chainDesc.BufferCount = s_kuiSwapChainBufferCount;
	chainDesc.Width = WindowManager::GetInstance()->GetWindowWidth();
	chainDesc.Height = WindowManager::GetInstance()->GetWindowHeight();
	chainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	chainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	chainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	chainDesc.SampleDesc.Count = 1;

	HRESULT hr = m_pDXGIFactory->CreateSwapChainForHwnd(m_pCommandQueue.Get(), WindowManager::GetInstance()->GetHWND(), &chainDesc, nullptr, nullptr, m_pSwapChain.GetAddressOf());

	if (FAILED(hr))
	{
		LOG_ERROR(tag, "Failed to create the swap chain!");

		return false;
	}

	//Create RTV heap
	D3D12_DESCRIPTOR_HEAP_DESC RTVHeapDesc = {};
	RTVHeapDesc.NumDescriptors = s_kuiSwapChainBufferCount;
	RTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	RTVHeapDesc.NodeMask = 0;

	HRESULT hr = m_pDevice->CreateDescriptorHeap(&RTVHeapDesc, IID_PPV_ARGS(m_pRTVHeap.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, "Failed to create the RTV heap!");

		return false;
	}

	//Create DSV heap
	D3D12_DESCRIPTOR_HEAP_DESC DSVHeapDesc = {};
	DSVHeapDesc.NumDescriptors = 1;
	DSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	DSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DSVHeapDesc.NodeMask = 0;

	hr = m_pDevice->CreateDescriptorHeap(&DSVHeapDesc, IID_PPV_ARGS(m_pDSVHeap.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, "Failed to create the DSV heap!");

		return false;
	}

	return true;
}

void App::FlushCommandQueue()
{
	// Advance the fence value to mark commands up to this fence point
	m_uiCurrentFence++;

	// Add an instruction to the command queue to set a new fence point
	HRESULT hr = m_pCommandQueue->Signal(m_pFence.Get(), m_uiCurrentFence);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, "Failed to create a new fence point!");

		return;
	}

	// Wait until the GPU has completed commands up to this fence point.
	if (m_pFence->GetCompletedValue() < m_uiCurrentFence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		if (eventHandle == 0)
		{
			LOG_ERROR(tag, "Failed to create event when fence point it hit!");

			return;
		}

		// Fire event when GPU hits current fence.  
		hr = m_pFence->SetEventOnCompletion(m_uiCurrentFence, eventHandle);

		if (FAILED(hr))
		{
			LOG_ERROR(tag, "Failed to set event for when fence point it hit!");

			return;
		}

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void App::LogAdapters()
{
	if (m_pDXGIFactory != nullptr)
	{
		UINT i = 0;
		IDXGIAdapter* pAdapter = nullptr;
		std::vector<IDXGIAdapter*> adapters;
		DXGI_ADAPTER_DESC desc;

		while (m_pDXGIFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND)
		{
			pAdapter->GetDesc(&desc);
			adapters.push_back(pAdapter);

			++i;
		}

		for (i = 0; i < adapters.size(); ++i)
		{
			LogAdapterOutputs(adapters[i], i == 0);
			adapters[i]->Release();
		}
	}
}

void App::LogAdapterOutputs(IDXGIAdapter* pAdapter, bool bSaveSettings)
{
	UINT i = 0;
	IDXGIOutput* pOutput = nullptr;

	while (pAdapter->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		pOutput->GetDesc(&desc);
		LogOutputInfo(pOutput, m_BackBufferFormat, bSaveSettings && i == 0);

		pOutput->Release();

		++i;
	}
}

void App::LogOutputInfo(IDXGIOutput* pOutput, DXGI_FORMAT format, bool bSaveSettings)
{
	UINT count = 0;
	UINT flags = 0;
	UINT i = 0;

	pOutput->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modes(count);
	pOutput->GetDisplayModeList(format, flags, &count, &modes[0]);

	if (bSaveSettings)
	{
		WindowManager::GetInstance()->SetWindowSettings(modes[i - 1]);
	}
}

ID3D12Resource* App::GetCurrentBackBuffer() const
{
	return m_pSwapChainBuffer[m_iBackBufferIndex].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE App::GetCurrentBackBufferView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_iBackBufferIndex, m_uiRTVDescSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE App::GetDepthStencilView() const
{
	return m_pDSVHeap->GetCPUDescriptorHandleForHeapStart();
}
