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

	// Reset the command list
	HRESULT hr = m_pGraphicsCommandList->Reset(m_pCommandAllocator.Get(), nullptr);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to reset the graphics command list!");
	}

	CreateGameObjects();

	if (CreateDescriptorHeaps() == false)
	{
		return false;
	}

	if (CreateRootSignature() == false)
	{
		return false;
	}

	CreateShadersAndUploadBuffers();

	CreateInputDescriptions();

	if (CreatePSOs() == false)
	{
		return false;
	}

	InitIMGUI();

	ExecuteCommandList();

	return true;
}

void BasicApp::Update(const Timer& kTimer)
{
	UploadBuffer<VisibleGameObjectCB>* pUploadBuffer = ShaderManager::GetInstance()->GetShaderConstantUploadBuffer<VisibleGameObjectCB>("VS");

	UINT uiCount = 0;


	VisibleGameObjectCB visibleGameObjectCB;

	GameObject* pGameObject = ObjectManager::GetInstance()->GetGameObject("Box2");
	pGameObject->Rotate(0.0f, 10.0f * kTimer.DeltaTime(), 0.0f);

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
	PIXBeginEvent(m_pGraphicsCommandList.Get(), PIX_COLOR(50, 50, 50), "Draw GameObjects");
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

	m_pGraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//Set the per frame constant buffer
	m_pGraphicsCommandList->SetGraphicsRootConstantBufferView(0, m_pPerFrameCB->Get()->GetGPUVirtualAddress());

	std::unordered_map<std::string, GameObject*>* pGameObjects = ObjectManager::GetInstance()->GetGameObjects();

	//Draw the gameobjects
	UINT uiPerObjByteSize = DirectXHelper::CalculatePaddedConstantBufferSize(sizeof(VisibleGameObjectCB));
	D3D12_GPU_VIRTUAL_ADDRESS perObjCBAddress = ShaderManager::GetInstance()->GetShaderConstantUploadBuffer<VisibleGameObjectCB>("VS")->Get()->GetGPUVirtualAddress();;

	for (std::unordered_map<std::string, GameObject*>::iterator it = pGameObjects->begin(); it != pGameObjects->end(); ++it)
	{
		//Set the per object constant buffer
		m_pGraphicsCommandList->SetGraphicsRootConstantBufferView(1, perObjCBAddress);

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

		perObjCBAddress += uiPerObjByteSize;
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
	pGameObject->Init("Box1", XMFLOAT4(0, 0, 10, 1), XMFLOAT3(5, 21, 11), XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f));

	pGameObject = new VisibleGameObject();
	pGameObject->Init("Box2", XMFLOAT4(-5, 0, 10, 1), XMFLOAT3(75, 44, 0), XMFLOAT4(3, 2, 1, 1));

	pGameObject = new VisibleGameObject();
	pGameObject->Init("Box3", XMFLOAT4(5, 0, 10, 1), XMFLOAT3(45, 45, 45), XMFLOAT4(1, 1, 1, 1));
}

void BasicApp::CreateShadersAndUploadBuffers()
{
	//Compile shaders
	ComPtr<ID3DBlob> pVertexShader;
	ComPtr<ID3DBlob> pPixelShader;

#if _DEBUG
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	//Create upload buffers
	UploadBuffer<VisibleGameObjectCB>* visibleCBUploadBuffer = new UploadBuffer<VisibleGameObjectCB>(m_pDevice.Get(), ObjectManager::GetInstance()->GetNumGameObjects(), true);

	m_pPerFrameCB = new UploadBuffer<PerFrameCB>(m_pDevice.Get(), 1, true);

	//Compile shaders
	pVertexShader = ShaderManager::GetInstance()->CompileShader<VisibleGameObjectCB>(L"Shaders/VertexShader.hlsl", "VS", nullptr, "main", "vs_5_0", visibleCBUploadBuffer);
	pPixelShader = ShaderManager::GetInstance()->CompileShader<VisibleGameObjectCB>(L"Shaders/PixelShader.hlsl", "PS", nullptr, "main", "ps_5_0", visibleCBUploadBuffer);
}

void BasicApp::CreateInputDescriptions()
{
	// Define the vertex input layout.
	m_VertexInputLayoutDesc =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

bool BasicApp::CreateRootSignature()
{
	//Create the root signature
	CD3DX12_DESCRIPTOR_RANGE range1;
	range1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE range2;
	range2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);

	CD3DX12_ROOT_PARAMETER slotRootParameter[2] = {};

	slotRootParameter[0].InitAsConstantBufferView(0);	//Per frame CB
	slotRootParameter[1].InitAsConstantBufferView(1);	//Per object CB

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init((UINT)2, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, signature.GetAddressOf(), error.GetAddressOf());

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

	return true;
}

bool BasicApp::CreateDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC IMGUIheapDescs;
	IMGUIheapDescs.NumDescriptors = 1;
	IMGUIheapDescs.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	IMGUIheapDescs.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	IMGUIheapDescs.NodeMask = 0;

	HRESULT hr = m_pDevice->CreateDescriptorHeap(&IMGUIheapDescs, IID_PPV_ARGS(&m_pIMGUIDescHeap));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the imgui buffer heap!");

		return false;
	}

	return true;
}

bool BasicApp::CreatePSOs()
{
	// Create the pipeline state object
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { m_VertexInputLayoutDesc.data(), (UINT)m_VertexInputLayoutDesc.size() };
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

	HRESULT hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pPipelineState.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the pipeline state object!");

		return false;
	}

	return true;
}

void BasicApp::InitIMGUI()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

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
