#include "BasicApp.h"
#include "Engine/Managers/ObjectManager.h"
#include "Engine/Managers/WindowManager.h"
#include "Engine/Helpers/DirectXHelper.h"
#include "Engine/GameObjects/VisibleGameObject.h"
#include "Engine/Cameras/DebugCamera.h"
#include "Engine/Managers/ShaderManager.h"
#include "Engine/Managers/MaterialManager.h"
#include "Engine/Managers/LightManager.h"
#include "Engine/Managers/TextureManager.h"
#include "Engine/DirectX/Light.h"

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

	CreateMaterials();
	CreateTextures();
	CreateGameObjects();

	CreateMaterialsUploadBuffer();

	if (CreateDescriptorHeaps() == false)
	{
		return false;
	}

	PopulateTextureHeap();

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

	m_Observer.Object = this;
	m_Observer.OnKeyDown = OnKeyDown;
	m_Observer.OnKeyHeld = nullptr;
	m_Observer.OnKeyUp = nullptr;

	InputManager::GetInstance()->Subscribe({ 48, 49, 50, 51 }, m_Observer);

	InitIMGUI();

	ExecuteCommandList();

	return true;
}

void BasicApp::Update(const Timer& kTimer)
{
	App::Update(kTimer);

	UploadBuffer<VisibleGameObjectCB>* pUploadBuffer = ShaderManager::GetInstance()->GetShaderConstantUploadBuffer<VisibleGameObjectCB>("VS");

	UINT uiCount = 0;

	VisibleGameObjectCB visibleGameObjectCB;

	GameObject* pGameObject = ObjectManager::GetInstance()->GetGameObject("Box1");
	pGameObject->Rotate(0.0f, 10.0f * kTimer.DeltaTime(), 0.0f);

	//GameObject* pGameObject = ObjectManager::GetInstance()->GetGameObject("Box2");
	//pGameObject->Rotate(0.0f, 10.0f * kTimer.DeltaTime(), 0.0f);

	for (std::unordered_map<std::string, GameObject*>::iterator it = ObjectManager::GetInstance()->GetGameObjects()->begin(); it != ObjectManager::GetInstance()->GetGameObjects()->end(); ++it)
	{
		it->second->Update(kTimer);

		XMMATRIX world = XMLoadFloat4x4(&it->second->GetWorldMatrix());

		visibleGameObjectCB.World = XMMatrixTranspose(world);

		world.r[3] = XMVectorSet(0, 0, 0, 1);

		visibleGameObjectCB.InvWorld = XMMatrixInverse(nullptr, world);

		pUploadBuffer->CopyData(uiCount, visibleGameObjectCB);

		uiCount++;
	}

	ObjectManager::GetInstance()->GetActiveCamera()->Update(kTimer);

	//Update per frame constant buffer
	PerFrameCB constants;

	XMMATRIX viewProj = XMLoadFloat4x4(&ObjectManager::GetInstance()->GetActiveCamera()->GetViewProjectionMatrix());

	//transpose the matrix to ensure it's in the right major
	constants.ViewProjection = XMMatrixTranspose(viewProj);

	//Zero out the translation component so it doesn't cause issues when transforming the normal
	viewProj.r[3] = XMVectorSet(0, 0, 0, 1);

	constants.InvTransposeViewProjection = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	Light* pLight = nullptr;

	//Update the light constant buffer
	for (int i = 0; i < MAX_LIGHTS; ++i)
	{
		constants.Lights[i] = *LightManager::GetInstance()->GetLight(i);
	}

	constants.Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1);
	constants.EyePosition = ObjectManager::GetInstance()->GetActiveCamera()->GetPosition();

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

	hr = m_pGraphicsCommandList->Reset(m_pCommandAllocator.Get(), m_pPipelineState);

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to reset the the graphics command list!");

		return;
	}

	PIX_ONLY(PIXBeginEvent(m_pGraphicsCommandList.Get(), PIX_COLOR(50, 50, 50), "Draw GameObjects"));

	CreateIMGUIWindow();

	m_pGraphicsCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());
	m_pGraphicsCommandList->RSSetViewports(1, &m_Viewport);
	m_pGraphicsCommandList->RSSetScissorRects(1, &m_ScissorRect);

	// Set the render target
	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Bind the depth buffer
	m_pGraphicsCommandList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), FALSE, &GetDepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_pTextureDescHeap.Get() };
	m_pGraphicsCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	const float clearColor[] = { 0.5f, 0.3f, 0.7f, 1.0f };

	m_pGraphicsCommandList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	m_pGraphicsCommandList->ClearRenderTargetView(GetCurrentBackBufferView(), clearColor, 0, nullptr);

	m_pGraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//Set the per frame constant buffer
	m_pGraphicsCommandList->SetGraphicsRootConstantBufferView(0, m_pPerFrameCB->Get()->GetGPUVirtualAddress());

	std::unordered_map<std::string, GameObject*>* pGameObjects = ObjectManager::GetInstance()->GetGameObjects();

	//Draw the gameobjects
	UINT uiPerObjByteSize = DirectXHelper::CalculatePaddedConstantBufferSize(sizeof(VisibleGameObjectCB));
	D3D12_GPU_VIRTUAL_ADDRESS perObjCBAddress = ShaderManager::GetInstance()->GetShaderConstantUploadBuffer<VisibleGameObjectCB>("VS")->Get()->GetGPUVirtualAddress();

	UINT uiMatByteSize = DirectXHelper::CalculatePaddedConstantBufferSize(sizeof(MaterialCB));
	D3D12_GPU_VIRTUAL_ADDRESS matCBAdress;

	CD3DX12_GPU_DESCRIPTOR_HANDLE textureAddress;

	std::vector<D3DTextureData*> pTextures;

	for (std::unordered_map<std::string, GameObject*>::iterator it = pGameObjects->begin(); it != pGameObjects->end(); ++it)
	{
		//Set the per object constant buffer
		m_pGraphicsCommandList->SetGraphicsRootConstantBufferView(1, perObjCBAddress);

		switch (it->second->GetType())
		{
		case GameObjectType::VISIBLE:
		{
			VisibleGameObject* pVisibleGameObject = (VisibleGameObject*)it->second;

			matCBAdress = m_pMaterialCB->Get()->GetGPUVirtualAddress() + uiMatByteSize * pVisibleGameObject->GetMaterial()->CBIndex;

			//Set the material constant buffer
			m_pGraphicsCommandList->SetGraphicsRootConstantBufferView(2, matCBAdress);

			pTextures = pVisibleGameObject->GetTextures();

			for (int i = 0; i < pTextures.size(); ++i)
			{
				textureAddress = m_pTextureDescHeap->GetGPUDescriptorHandleForHeapStart();
				textureAddress.Offset(pTextures[i]->HeapIndex, m_uiCBVSRVDescSize);

				m_pGraphicsCommandList->SetGraphicsRootDescriptorTable(3 + i, textureAddress);
			}

			pVisibleGameObject->Draw();

			break;
		}

		default:
			break;
		}

		perObjCBAddress += uiPerObjByteSize;
	}

	PIX_ONLY(PIXBeginEvent(m_pGraphicsCommandList.Get(), PIX_COLOR(50, 50, 50), "Draw IMGUI"));

	ID3D12DescriptorHeap* pIMGUIDescriptorHeaps[] = { m_pIMGUIDescHeap.Get() };
	m_pGraphicsCommandList->SetDescriptorHeaps(_countof(pIMGUIDescriptorHeaps), pIMGUIDescriptorHeaps);

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_pGraphicsCommandList.Get());

	PIX_ONLY(PIXEndEvent(m_pGraphicsCommandList.Get()));

	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	PIX_ONLY(PIXEndEvent(m_pGraphicsCommandList.Get()));

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
	//pGameObject->Init("Box1", XMFLOAT3(0, 0, 10), XMFLOAT3(5, 21, 11), XMFLOAT3(0.2f, 0.2f, 0.2f), "test");
	pGameObject->Init("Box1", XMFLOAT3(0, 0, 10), XMFLOAT3(0, 0, 0), XMFLOAT3(3, 3, 3), "test", { "color", "normal", "height" });

	//pGameObject = new VisibleGameObject();
	//pGameObject->Init("Box2", XMFLOAT3(-5, 0, 10), XMFLOAT3(75, 44, 0), XMFLOAT3(3, 2, 1), "test");

	//pGameObject = new VisibleGameObject();
	//pGameObject->Init("Box3", XMFLOAT3(5, 0, 10), XMFLOAT3(45, 45, 45), XMFLOAT3(1, 1, 1), "test");

	pGameObject = new VisibleGameObject();
	pGameObject->Init("Light", XMFLOAT3(0, 0, 0), XMFLOAT3(0, 0, 0), XMFLOAT3(0.2f, 0.2f, 0.2f), "test", { "color" });

	Light* pPointLight = new PointLight(XMFLOAT3(0, 0, 0), XMFLOAT3(0.2f, 0.2f, 0.2f), XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT4(0.5f, 0.5f, 0.5f, 10.0f), XMFLOAT3(0.2f, 0.09f, 0.0f), 1000.0f);
	Light* pSpotLight = new SpotLight(XMFLOAT3(), XMFLOAT3(0.2f, 0.2f, 0.2f), XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT4(0.5f, 0.5f, 0.5f, 10.0f), XMFLOAT3(0.2f, 0.09f, 0.0f), 1000.0f, XMFLOAT4(0, 0, 1, 0), 45.0f);
	Light* pDirectionalLight = new DirectionalLight(XMFLOAT3(0.2f, 0.2f, 0.2f), XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT4(0.5f, 0.5f, 0.5f, 10), XMFLOAT4(0, 0, 1, 0));
	LightManager::GetInstance()->AddLight(pPointLight);
	//LightManager::GetInstance()->AddLight(pPointLight);
}

void BasicApp::CreateMaterials()
{
	Material* pMat = new Material();
	pMat->name = "test";
	pMat->Ambient = XMFLOAT4(0.75f, 0.75f, 0.75f, 1.0f);
	pMat->Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 0.745f);
	pMat->Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 10.0f);

	MaterialManager::GetInstance()->AddMaterial(pMat);
}

void BasicApp::CreateTextures()
{
	TextureManager::GetInstance()->AddTexture("color", L"Textures/bricks_COLOR.dds");
	TextureManager::GetInstance()->AddTexture("normal", L"Textures/bricks_NORM.dds");
	TextureManager::GetInstance()->AddTexture("height", L"Textures/bricks_DISP.dds");

	//TextureManager::GetInstance()->AddTexture("color", L"Textures/Crate_COLOR.dds");
	//TextureManager::GetInstance()->AddTexture("normal", L"Textures/Crate_NORM.dds");
	//TextureManager::GetInstance()->AddTexture("height", L"Textures/Crate_DISP.dds");
}

void BasicApp::CreateMaterialsUploadBuffer()
{
	m_pMaterialCB = new UploadBuffer<MaterialCB>(m_pDevice.Get(), (UINT)MaterialManager::GetInstance()->GetMaterials()->size(), true);

	UINT uiCount = 0;

	MaterialCB mat;

	for (std::unordered_map<std::string, Material*>::iterator it = MaterialManager::GetInstance()->GetMaterials()->begin(); it != MaterialManager::GetInstance()->GetMaterials()->end(); ++it)
	{
		it->second->CBIndex = uiCount;

		mat.Ambient = it->second->Ambient;
		mat.Diffuse = it->second->Diffuse;
		mat.Specular = it->second->Specular;

		m_pMaterialCB->CopyData(uiCount, mat);

		uiCount++;
	}
}

void BasicApp::CreateShadersAndUploadBuffers()
{
#if _DEBUG
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	//Create upload buffers
	UploadBuffer<VisibleGameObjectCB>* visibleCBUploadBuffer = new UploadBuffer<VisibleGameObjectCB>(m_pDevice.Get(), ObjectManager::GetInstance()->GetNumGameObjects(), true);

	m_pPerFrameCB = new UploadBuffer<PerFrameCB>(m_pDevice.Get(), 1, true);

	D3D_SHADER_MACRO normal[] = 
	{ 
		"NORMAL_MAPPING", "1",
		NULL, NULL
	};

	D3D_SHADER_MACRO parallax[] = 
	{
		"PARALLAX_MAPPING", "1",
		NULL, NULL
	};

	D3D_SHADER_MACRO parallaxOcclusion[] = 
	{
		"PARALLAX_OCCLUSION", "1",
		NULL, NULL
	};

	//Compile shaders
	ShaderManager::GetInstance()->CompileShaderVS<VisibleGameObjectCB>(L"Shaders/VertexShader.hlsl", "VS", nullptr, "VSMain", "vs_5_0", visibleCBUploadBuffer);
	ShaderManager::GetInstance()->CompileShaderPS<VisibleGameObjectCB>(L"Shaders/VertexShader.hlsl", "PS_Nothing", nullptr, "PSMain", "ps_5_0", visibleCBUploadBuffer);
	ShaderManager::GetInstance()->CompileShaderPS<VisibleGameObjectCB>(L"Shaders/VertexShader.hlsl", "PS_Normal", normal, "PSMain", "ps_5_0", visibleCBUploadBuffer);
	ShaderManager::GetInstance()->CompileShaderPS<VisibleGameObjectCB>(L"Shaders/VertexShader.hlsl", "PS_Parallax", parallax, "PSMain", "ps_5_0", visibleCBUploadBuffer);
	ShaderManager::GetInstance()->CompileShaderPS<VisibleGameObjectCB>(L"Shaders/VertexShader.hlsl", "PS_ParallaxOcclusion", parallaxOcclusion, "PSMain", "ps_5_0", visibleCBUploadBuffer);
}

void BasicApp::CreateInputDescriptions()
{
	// Define the vertex input layout.
	m_VertexInputLayoutDesc =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> BasicApp::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}

bool BasicApp::CreateRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE albedoTable;
	albedoTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)1, 3);

	CD3DX12_DESCRIPTOR_RANGE normalTable;
	normalTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)1, 4);

	CD3DX12_DESCRIPTOR_RANGE heightTable;
	heightTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)1, 5);

	CD3DX12_ROOT_PARAMETER slotRootParameter[6] = {};

	slotRootParameter[0].InitAsConstantBufferView(0);	//Per frame CB
	slotRootParameter[1].InitAsConstantBufferView(1);	//Per object CB
	slotRootParameter[2].InitAsConstantBufferView(2);	//Material CB

	//Adding textures
	slotRootParameter[3].InitAsDescriptorTable(1, &albedoTable);
	slotRootParameter[4].InitAsDescriptorTable(1, &normalTable);
	slotRootParameter[5].InitAsDescriptorTable(1, &heightTable);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> staticSamplers = GetStaticSamplers();



	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init((UINT)_countof(slotRootParameter), slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
	D3D12_DESCRIPTOR_HEAP_DESC IMGUIheapDescs = {};
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

	D3D12_DESCRIPTOR_HEAP_DESC srvDesc = {};
	srvDesc.NumDescriptors = (UINT)TextureManager::GetInstance()->GetTextures()->size();
	srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	hr = m_pDevice->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&m_pTextureDescHeap));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the texture buffer heap!");

		return false;
	}

	return true;
}

void BasicApp::PopulateTextureHeap()
{
	std::unordered_map<std::string, D3DTextureData*>* pTextures = TextureManager::GetInstance()->GetTextures();

	CD3DX12_CPU_DESCRIPTOR_HANDLE descHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pTextureDescHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	UINT uiCount = 0;

	for (std::unordered_map<std::string, D3DTextureData*>::iterator it = pTextures->begin(); it != pTextures->end(); ++it)
	{
		//Cerate shader resource view
		srvDesc.Texture2D.MipLevels = it->second->Resource->GetDesc().MipLevels;
		srvDesc.Format = it->second->Resource->GetDesc().Format;

		m_pDevice->CreateShaderResourceView(it->second->Resource.Get(), &srvDesc, descHandle);

		//Offset handle ready for next view
		descHandle.Offset(1, m_uiCBVSRVDescSize);

		it->second->HeapIndex = uiCount;

		++uiCount;
	}
}

bool BasicApp::CreatePSOs()
{
	//Initialise constant pipeline desc propertires
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { m_VertexInputLayoutDesc.data(), (UINT)m_VertexInputLayoutDesc.size() };
	psoDesc.pRootSignature = m_pRootSignature.Get();
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

	Microsoft::WRL::ComPtr<ID3D12PipelineState>* ppPipelineState;

	PSODesc psoDescription;

	for (std::unordered_map<std::string, void*>::iterator itA = ShaderManager::GetInstance()->GetShaders()->begin(); itA != ShaderManager::GetInstance()->GetShaders()->end(); ++itA)
	{
		if (static_cast<Shader<VisibleGameObjectCB>*>(itA->second)->GetShaderType() != ShaderType::Vertex)
		{
			continue;
		}

		for (std::unordered_map<std::string, void*>::iterator itB = ShaderManager::GetInstance()->GetShaders()->begin(); itB != ShaderManager::GetInstance()->GetShaders()->end(); ++itB)
		{
			if (static_cast<Shader<VisibleGameObjectCB>*>(itB->second)->GetShaderType() != ShaderType::Pixel)
			{
				continue;
			}

			psoDesc.VS = CD3DX12_SHADER_BYTECODE(ShaderManager::GetInstance()->GetShader<VisibleGameObjectCB>(itA->first)->GetShaderBlob().Get());
			psoDesc.PS = CD3DX12_SHADER_BYTECODE(ShaderManager::GetInstance()->GetShader<VisibleGameObjectCB>(itB->first)->GetShaderBlob().Get());

			ppPipelineState = new Microsoft::WRL::ComPtr<ID3D12PipelineState>();

			HRESULT hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(ppPipelineState->GetAddressOf()));

			if (FAILED(hr))
			{
				LOG_ERROR(tag, L"Failed to create the pipeline state object!");

				return false;
			}

			psoDescription.VSName = itA->first;
			psoDescription.PSName = itB->first;

			m_PipelineStates[psoDescription] = ppPipelineState->Get();
		}
	}

	PSODesc desc = { "VS", "PS_ParallaxOcclusion" };

	m_pPipelineState = m_PipelineStates[desc].Get();

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

void BasicApp::OnKeyDown(void* pObject, int iKeycode)
{
	BasicApp* pBasicApp = (BasicApp*)pObject;

	PSODesc psoDesc;
	psoDesc.VSName = "VS";

	switch (iKeycode)
	{
	case 48: //0
		psoDesc.PSName = "PS_Nothing";
		pBasicApp->m_pPipelineState = pBasicApp->m_PipelineStates[psoDesc].Get();
		break;

	case 49: //1
		psoDesc.PSName = "PS_Normal";
		pBasicApp->m_pPipelineState = pBasicApp->m_PipelineStates[psoDesc].Get();
		break;

	case 50: //2
		psoDesc.PSName = "PS_Parallax";
		pBasicApp->m_pPipelineState = pBasicApp->m_PipelineStates[psoDesc].Get();
		break;

	case 51: //3
		psoDesc.PSName = "PS_ParallaxOcclusion";
		pBasicApp->m_pPipelineState = pBasicApp->m_PipelineStates[psoDesc].Get();
		break;

	}
}
