#include "BasicApp.h"
#include "Engine/Managers/ObjectManager.h"
#include "Engine/Managers/WindowManager.h"
#include "Engine/Helpers/DirectXHelper.h"
#include "Engine/GameObjects/VisibleGameObject.h"
#include "Engine/Cameras/DebugCamera.h"
#include "Engine/Managers/ShaderManager.h"

#if PIX
#include "pix3.h"
#endif

#include "imgui\imgui.h"
#include "imgui\imgui_impl_win32.h"
#include "imgui\imgui_impl_dx12.h"

#include <DirectX/d3dx12.h>
#include <d3dcompiler.h>

using namespace DirectX;
using namespace Microsoft::WRL;

Tag tag = L"BasicApp";

BasicApp::BasicApp(HINSTANCE hInstance) : App(hInstance)
{
}

bool BasicApp::Init()
{
	if (App::Init() == false)
	{
		return false;
	}

	ResetCommmandList();

	CreateGameObjects();

	//Create const buffer heap
	D3D12_DESCRIPTOR_HEAP_DESC heapDescs;
	heapDescs.NumDescriptors = ObjectManager::GetInstance()->GetNumGameObjects() + 1;	//Need to add 1 for the per frame constant buffer
	heapDescs.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDescs.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDescs.NodeMask = 0;

	HRESULT hr = m_pDevice->CreateDescriptorHeap(&heapDescs, IID_PPV_ARGS(&m_pConstDescHeap));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the const buffer heap!");

		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC IMGUIheapDescs;
	IMGUIheapDescs.NumDescriptors = 1;
	IMGUIheapDescs.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	IMGUIheapDescs.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	IMGUIheapDescs.NodeMask = 0;

	hr = m_pDevice->CreateDescriptorHeap(&IMGUIheapDescs, IID_PPV_ARGS(&m_pIMGUIDescHeap));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the imgui buffer heap!");

		return false;
	}

	InitIMGUI();

	//Create the root signature
	CD3DX12_DESCRIPTOR_RANGE range1;
	range1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE range2;
	range2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	CD3DX12_ROOT_PARAMETER slotRootParameter[2];

	slotRootParameter[0].InitAsDescriptorTable(1, &range1);
	slotRootParameter[1].InitAsDescriptorTable(1, &range2);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(static_cast<uint32_t>(2), slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, signature.GetAddressOf(), error.GetAddressOf());

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the serialize root signature!");

		return false;
	}

	hr = m_pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_pRootSignature.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the root signature!");

		return false;
	}

	//Compile shaders
	ComPtr<ID3DBlob> pVertexShader;
	ComPtr<ID3DBlob> pPixelShader;

#if _DEBUG
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	UploadBuffer<VisibleGameObjectCB>* visibleCBUploadBuffer = new UploadBuffer<VisibleGameObjectCB>(m_pDevice.Get(), 1, true);

	pVertexShader = ShaderManager::GetInstance()->CompileShader<VisibleGameObjectCB>(L"Shaders/VertexShader.hlsl", "VS", nullptr, "main", "vs_5_0", visibleCBUploadBuffer);
	pPixelShader = ShaderManager::GetInstance()->CompileShader<VisibleGameObjectCB>(L"Shaders/PixelShader.hlsl", "PS", nullptr, "main", "ps_5_0", visibleCBUploadBuffer);

	//Create views to the constant buffer

	UINT uiPerObjCBSize = DirectXHelper::CalculatePaddedConstantBufferSize(sizeof(VisibleGameObjectCB));

	for (UINT i = 0; i < ObjectManager::GetInstance()->GetNumGameObjects(); ++i)	//Create the per object constant buffer views first
	{
		D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = ShaderManager::GetInstance()->GetShaderConstantUploadBuffer<VisibleGameObjectCB>("VS")->Get()->GetGPUVirtualAddress();

		gpuAddress += (UINT64)i * (UINT64)uiPerObjCBSize;

		CD3DX12_CPU_DESCRIPTOR_HANDLE descHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pConstDescHeap->GetCPUDescriptorHandleForHeapStart());
		descHandle.Offset(i, m_uiCBVSRVDescSize);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = gpuAddress;
		cbvDesc.SizeInBytes = uiPerObjCBSize;

		m_pDevice->CreateConstantBufferView(&cbvDesc, descHandle);
	}

	//Now create the per frame constant buffer at the end of the heap
	m_pPerFrameCB = new UploadBuffer<PerFrameCB>(m_pDevice.Get(), 1, true);

	UINT uiPerFrameCBSize = DirectXHelper::CalculatePaddedConstantBufferSize(sizeof(PerFrameCB));

	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = m_pPerFrameCB->Get()->GetGPUVirtualAddress();

	CD3DX12_CPU_DESCRIPTOR_HANDLE descHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pConstDescHeap->GetCPUDescriptorHandleForHeapStart());
	descHandle.Offset(ObjectManager::GetInstance()->GetNumGameObjects(), m_uiCBVSRVDescSize);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = gpuAddress;
	cbvDesc.SizeInBytes = uiPerFrameCBSize;

	m_pDevice->CreateConstantBufferView(&cbvDesc, descHandle);

	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// Create the pipeline state object
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_pRootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(ShaderManager::GetInstance()->GetShader<VisibleGameObjectCB>("VS")->GetShaderBlob().Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(ShaderManager::GetInstance()->GetShader<VisibleGameObjectCB>("PS")->GetShaderBlob().Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_BackBufferFormat;
	psoDesc.SampleDesc.Count = m_b4xMSAAState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m_b4xMSAAState ? (m_uiMSAAQuality - 1) : 0;
	psoDesc.DSVFormat = m_DepthStencilFormat;

	hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pPipelineState.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the pipeline state object!");

		return false;
	}

	ExecuteCommandList();

	return true;
}

void BasicApp::Update(const Timer& kTimer)
{
	UploadBuffer<VisibleGameObjectCB>* pUploadBuffer = ShaderManager::GetInstance()->GetShaderConstantUploadBuffer<VisibleGameObjectCB>("VS");

	UINT uiCount = 0;


	VisibleGameObjectCB visibleGameObjectCB;

	for (std::unordered_map<std::string, GameObject*>::iterator it = ObjectManager::GetInstance()->GetGameObjects()->begin(); it != ObjectManager::GetInstance()->GetGameObjects()->end(); ++it)
	{
		it->second->Update(kTimer);

		visibleGameObjectCB.World = XMMatrixTranspose(XMLoadFloat4x4(&it->second->GetWorldMatrix()));

		pUploadBuffer->CopyData(uiCount, visibleGameObjectCB);

		uiCount++;
	}

	ObjectManager::GetInstance()->GetActiveCamera()->Update(kTimer);

	//Update per frame constant buffer
	PerFrameCB constants;

	//transpose the matrix to ensure it's in the right major
	constants.ViewProjection = XMMatrixTranspose(XMLoadFloat4x4(&ObjectManager::GetInstance()->GetActiveCamera()->GetViewProjectionMatrix()));
	m_pPerFrameCB->CopyData(0, constants);
}

void BasicApp::Draw()
{
	HRESULT hr = m_pCommandAllocator->Reset();

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to reset the the command list allocator!");

		return;
	}

	hr = m_pGraphicsCommandList->Reset(m_pCommandAllocator.Get(), m_pPipelineState.Get());

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to reset the the graphics command list!");

		return;
	}

#if PIX
	PIXBeginEvent(m_pGraphicsCommandList.Get(), PIX_COLOR(50, 50, 50), "TEST");
#endif

	CreateIMGUIWindow();

	m_pGraphicsCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	m_pGraphicsCommandList->RSSetViewports(1, &m_Viewport);
	m_pGraphicsCommandList->RSSetScissorRects(1, &m_ScissorRect);

	// Set the render target
	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Bind the depth buffer
	m_pGraphicsCommandList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), FALSE, &GetDepthStencilView());

	const float clearColor[] = { 0.5f, 0.3f, 0.7f, 1.0f };

	m_pGraphicsCommandList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	m_pGraphicsCommandList->ClearRenderTargetView(GetCurrentBackBufferView(), clearColor, 0, nullptr);

	ID3D12DescriptorHeap* pDescriptorHeaps[] = { m_pConstDescHeap.Get() };
	m_pGraphicsCommandList->SetDescriptorHeaps(_countof(pDescriptorHeaps), pDescriptorHeaps);

	m_pGraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//Set the per frame constant buffer
	CD3DX12_GPU_DESCRIPTOR_HANDLE perFrameCBOffset = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pConstDescHeap->GetGPUDescriptorHandleForHeapStart());
	perFrameCBOffset.Offset(ObjectManager::GetInstance()->GetNumGameObjects(), m_uiCBVSRVDescSize);
	m_pGraphicsCommandList->SetGraphicsRootDescriptorTable(0, perFrameCBOffset);

	std::unordered_map<std::string, GameObject*>* pGameObjects = ObjectManager::GetInstance()->GetGameObjects();

	//Draw the gameobjects
	UINT uiCount = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE constDescHeapHandle = m_pConstDescHeap->GetGPUDescriptorHandleForHeapStart();

	for (std::unordered_map<std::string, GameObject*>::iterator it = pGameObjects->begin(); it != pGameObjects->end(); ++it)
	{
		// Offset to the CBV in the descriptor heap for this object and for this frame resource.
		CD3DX12_GPU_DESCRIPTOR_HANDLE handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(constDescHeapHandle);
		handle.Offset(uiCount, m_uiCBVSRVDescSize);

		m_pGraphicsCommandList->SetGraphicsRootDescriptorTable(1, handle);

		switch (it->second->GetType())
		{
		case GameObjectType::VISIBLE:
		{
			VisibleGameObject* pVisibleGameObject = (VisibleGameObject*)it->second;

			pVisibleGameObject->Draw();

			break;
		}

		default:
			break;
		}

		uiCount++;
	}

	ID3D12DescriptorHeap* pIMGUIDescriptorHeaps[] = { m_pIMGUIDescHeap.Get() };
	m_pGraphicsCommandList->SetDescriptorHeaps(_countof(pIMGUIDescriptorHeaps), pIMGUIDescriptorHeaps);

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_pGraphicsCommandList.Get());

	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

#if PIX
	PIXEndEvent(m_pGraphicsCommandList.Get());
#endif

	hr = m_pGraphicsCommandList->Close();

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to close the graphics command list!");

		return;
	}

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_pGraphicsCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	hr = m_pSwapChain->Present(0, 0);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to present the current frame!");

		return;
	}

	m_iBackBufferIndex = (m_iBackBufferIndex + 1) % s_kuiSwapChainBufferCount;

	FlushCommandQueue();
}

void BasicApp::Load()
{
}

void BasicApp::ResetCommmandList()
{
	// Reset the command list
	HRESULT hr = m_pGraphicsCommandList->Reset(m_pCommandAllocator.Get(), nullptr);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to reset the graphics command list!");
	}
}

void BasicApp::ExecuteCommandList()
{
	//Execute Init command
	HRESULT hr = m_pGraphicsCommandList->Close();

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to close the graphics command list!");
	}

	ID3D12CommandList* commandLists[] = { m_pGraphicsCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	FlushCommandQueue();
}

void BasicApp::CreateGameObjects()
{
	//Create camera
	Camera* pCamera = new DebugCamera(XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, 1), XMFLOAT3(0, 1, 0), 0.1f, 1000.0f, "BasicCamera");
	ObjectManager::GetInstance()->SetActiveCamera(pCamera);

	VisibleGameObject* pGameObject = new VisibleGameObject();
	pGameObject->Init("Box", XMFLOAT4(10, 0, 0, 1), XMFLOAT3(0, 90, 0), XMFLOAT4(2, 1, 1, 1));
}

void BasicApp::InitIMGUI()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(WindowManager::GetInstance()->GetHWND());
	ImGui_ImplDX12_Init(m_pDevice.Get(),
		s_kuiSwapChainBufferCount,
		m_BackBufferFormat, m_pIMGUIDescHeap.Get(),
		m_pIMGUIDescHeap->GetCPUDescriptorHandleForHeapStart(),
		m_pIMGUIDescHeap->GetGPUDescriptorHandleForHeapStart());
}

void BasicApp::CreateIMGUIWindow()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

	ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
	ImGui::Checkbox("Demo Window", &m_bShowDemoWindow);      // Edit bools storing our window open/close state

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();

	if (m_bShowDemoWindow == true)
	{
		ImGui::ShowDemoWindow(&m_bShowDemoWindow);
	}
}
