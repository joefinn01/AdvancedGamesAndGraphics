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
#include "Engine/DirectX/Vertices.h"

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

	if (CreateRootSignatures() == false)
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

	m_MovementObserver.Object = this;
	m_MovementObserver.OnKeyDown = nullptr;
	m_MovementObserver.OnKeyHeld = OnKeyHeld;
	m_MovementObserver.OnKeyUp = nullptr;

	InputManager::GetInstance()->Subscribe({ VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, 73, 74, 75, 76 }, m_MovementObserver);
	InputManager::GetInstance()->Subscribe({ 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 69, 81 }, m_Observer);

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

	GameObject* pGameObject = ObjectManager::GetInstance()->GetGameObject("Light");
	pGameObject->SetPosition(LightManager::GetInstance()->GetLight("point")->lightCB.Position);

	if (m_bRotateCube == true)
	{
		pGameObject = ObjectManager::GetInstance()->GetGameObject("Box1");
		pGameObject->Rotate(0.0f, 10.0f * kTimer.DeltaTime(), 0.0f);
	}

	for (std::unordered_map<std::string, GameObject*>::iterator it = ObjectManager::GetInstance()->GetGameObjects()->begin(); it != ObjectManager::GetInstance()->GetGameObjects()->end(); ++it)
	{
		it->second->Update(kTimer);

		XMMATRIX world = XMLoadFloat4x4(&it->second->GetWorldMatrix());

		XMStoreFloat4x4(&visibleGameObjectCB.World, XMMatrixTranspose(world));

		world.r[3] = XMVectorSet(0, 0, 0, 1);

		XMStoreFloat4x4(&visibleGameObjectCB.InvWorld, XMMatrixInverse(nullptr, world));

		pUploadBuffer->CopyData(uiCount, visibleGameObjectCB);

		uiCount++;
	}

	ObjectManager::GetInstance()->GetActiveCamera()->Update(kTimer);

	//Update g buffer per frame CB
	GBufferPerFrameCB gBufferPerFrameCB;

	XMMATRIX viewProj = XMLoadFloat4x4(&ObjectManager::GetInstance()->GetActiveCamera()->GetViewProjectionMatrix());

	gBufferPerFrameCB.EyePosW = ObjectManager::GetInstance()->GetActiveCamera()->GetPosition();

	XMStoreFloat4x4(&gBufferPerFrameCB.ViewProjection, XMMatrixTranspose(viewProj));

	m_pGBufferPerFrameCB->CopyData(0, gBufferPerFrameCB);

	//Update light pass per frame CB
	LightPassPerFrameCB lightPassPerFrameCB;
	lightPassPerFrameCB.EyePosition = ObjectManager::GetInstance()->GetActiveCamera()->GetPosition();

	XMStoreFloat4x4(&lightPassPerFrameCB.InvViewProjection, XMMatrixTranspose(XMMatrixInverse(nullptr, viewProj)));

	m_pLightPassPerFrameCB->CopyData(0, lightPassPerFrameCB);
}

void BasicApp::Draw()
{
	HRESULT hr = m_pCommandAllocator->Reset();

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to reset the the command list allocator!");

		return;
	}

	PSODesc GBufferPSODesc = { "VS_GBuffer", m_sCurrentGBufferPSName };

	hr = m_pGraphicsCommandList->Reset(m_pCommandAllocator.Get(), m_PipelineStates[GBufferPSODesc].Get());

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to reset the the graphics command list!");

		return;
	}

	PIX_ONLY(PIXBeginEvent(m_pGraphicsCommandList.Get(), PIX_COLOR(50, 50, 50), "Draw GameObjects"));

	CreateIMGUIWindow();

	PopulateGBuffer();
	DoLightPass();
	DoPostProcessing();

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

void BasicApp::OnResize()
{
	App::OnResize();

	if (m_pPostProcessingPerFrameCB != nullptr)
	{
		UpdatePostProcessingCB();
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
	Light* pDirectionalLight = new DirectionalLight(XMFLOAT3(0.2f, 0.2f, 0.2f), XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT4(0.5f, 0.5f, 0.5f, 10), XMFLOAT4(0, 0, 1, 0), false);
	LightManager::GetInstance()->AddLight("point", pPointLight);
	LightManager::GetInstance()->AddLight("spot", pSpotLight);
	LightManager::GetInstance()->AddLight("direction", pDirectionalLight);

	//Create screen quad
	std::array<ScreenQuadVertex, 4> vertices =
	{
		ScreenQuadVertex(XMFLOAT3(-1, 1, 0), XMFLOAT2(0, 0)),	//Top left
		ScreenQuadVertex(XMFLOAT3(1, 1, 0), XMFLOAT2(1, 0)),	//Top right
		ScreenQuadVertex(XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 1)),	//Bottom left
		ScreenQuadVertex(XMFLOAT3(1, -1, 0), XMFLOAT2(1, 1))	//Bottom right
	};

	m_pScreenQuadVertexBufferGPU = DirectXHelper::CreateDefaultBuffer(m_pDevice.Get(),
		m_pGraphicsCommandList.Get(),
		vertices.data(),
		(UINT)sizeof(Vertex) * 4,
		m_pScreenQuadVertexBufferUploader);

	m_ScreenQuadVBView.BufferLocation = m_pScreenQuadVertexBufferGPU->GetGPUVirtualAddress();
	m_ScreenQuadVBView.StrideInBytes = sizeof(ScreenQuadVertex);
	m_ScreenQuadVBView.SizeInBytes = (UINT)sizeof(ScreenQuadVertex) * 4;
}

void BasicApp::CreateMaterials()
{
	Material* pMat = new Material();
	pMat->name = "test";
	pMat->Ambient = XMFLOAT4(0.75f, 0.75f, 0.75f, 1.0f);
	pMat->Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
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

	m_pGBufferPerFrameCB = new UploadBuffer<GBufferPerFrameCB>(m_pDevice.Get(), 1, true);
	m_pLightPassPerFrameCB = new UploadBuffer<LightPassPerFrameCB>(m_pDevice.Get(), 1, true);
	m_pPostProcessingPerFrameCB = new UploadBuffer<PostProcessingPerFrameCB>(m_pDevice.Get(), 1, true);

	//Populate post processing constant buffer
	PostProcessingPerFrameCB postProcessingPerFrameCB;
	postProcessingPerFrameCB.ScreenWidth = (int)WindowManager::GetInstance()->GetWindowWidth();
	postProcessingPerFrameCB.ScreenHeight = (int)WindowManager::GetInstance()->GetWindowHeight();

	m_pPostProcessingPerFrameCB->CopyData(0, postProcessingPerFrameCB);

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
	
	D3D_SHADER_MACRO parallaxShadow[] = 
	{
		"PARALLAX_SHADOW", "1",
		NULL, NULL
	};

	//Compile shaders
	ShaderManager::GetInstance()->CompileShaderVS<VisibleGameObjectCB>(L"Shaders/VertexShaders/VertexShader.hlsl", "VS", nullptr, "VSMain", "vs_5_0", visibleCBUploadBuffer);
	ShaderManager::GetInstance()->CompileShaderVS<VisibleGameObjectCB>(L"Shaders/VertexShaders/ScreenQuadVS.hlsl", "VS_ScreenQuad", nullptr, "main", "vs_5_0", visibleCBUploadBuffer);
	ShaderManager::GetInstance()->CompileShaderVS<VisibleGameObjectCB>(L"Shaders/VertexShaders/GBufferVS.hlsl", "VS_GBuffer", nullptr, "main", "vs_5_0", visibleCBUploadBuffer);

	ShaderManager::GetInstance()->CompileShaderPS<VisibleGameObjectCB>(L"Shaders/PixelShaders/GBufferPS.hlsl", "PS_GBuffer_Nothing", nullptr, "main", "ps_5_0", visibleCBUploadBuffer);
	ShaderManager::GetInstance()->CompileShaderPS<VisibleGameObjectCB>(L"Shaders/PixelShaders/GBufferPS.hlsl", "PS_GBuffer_Normal", normal, "main", "ps_5_0", visibleCBUploadBuffer);
	ShaderManager::GetInstance()->CompileShaderPS<VisibleGameObjectCB>(L"Shaders/PixelShaders/GBufferPS.hlsl", "PS_GBuffer_Parallax", parallax, "main", "ps_5_0", visibleCBUploadBuffer);
	ShaderManager::GetInstance()->CompileShaderPS<VisibleGameObjectCB>(L"Shaders/PixelShaders/GBufferPS.hlsl", "PS_GBuffer_ParallaxOcclusion", parallaxOcclusion, "main", "ps_5_0", visibleCBUploadBuffer);
	ShaderManager::GetInstance()->CompileShaderPS<VisibleGameObjectCB>(L"Shaders/PixelShaders/GBufferPS.hlsl", "PS_GBuffer_ParallaxShadow", parallaxShadow, "main", "ps_5_0", visibleCBUploadBuffer);
	ShaderManager::GetInstance()->CompileShaderPS<VisibleGameObjectCB>(L"Shaders/PixelShaders/LightPassPS.hlsl", "PS_LightPass_Nothing", nullptr, "main", "ps_5_0", visibleCBUploadBuffer);
	ShaderManager::GetInstance()->CompileShaderPS<VisibleGameObjectCB>(L"Shaders/PixelShaders/LightPassPS.hlsl", "PS_LightPass_ParallaxShadow", parallaxShadow, "main", "ps_5_0", visibleCBUploadBuffer);
	ShaderManager::GetInstance()->CompileShaderPS<VisibleGameObjectCB>(L"Shaders/PixelShaders/PostProcessingPS.hlsl", "PS_PostProcessing", nullptr, "main", "ps_5_0", visibleCBUploadBuffer);
}

void BasicApp::CreateInputDescriptions()
{
	// Define the vertex input layout.
	m_GBufferVertexInputLayoutDesc =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	m_ScreenQuadVertexInputLayoutDesc =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
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

bool BasicApp::CreateRootSignatures()
{
	if (CreateGBufferRootSignature() == false)
	{
		return false;
	}

	if (CreateLightPassRootSignature() == false)
	{
		return false;
	}

	if (CreatePostProcessingRootSignature() == false)
	{
		return false;
	}

	return true;
}

bool BasicApp::CreateGBufferRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE albedoTable;
	albedoTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)1, 0);

	CD3DX12_DESCRIPTOR_RANGE normalTable;
	normalTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)1, 1);

	CD3DX12_DESCRIPTOR_RANGE heightTable;
	heightTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)1, 2);

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
		LOG_ERROR(tag, L"Failed to create GBuffer serialize root signature!");

		return false;
	}

	hr = m_pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_pGBufferSignature.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create GBuffer root signature!");

		return false;
	}

	return true;
}

bool BasicApp::CreateLightPassRootSignature()
{
	//+2 for constant buffers and +1 for depth texture
	CD3DX12_ROOT_PARAMETER slotRootParameter[GBUFFER_NUM + 3] = {};

	slotRootParameter[0].InitAsConstantBufferView(0, 0U, D3D12_SHADER_VISIBILITY_PIXEL);	//Per frame CB
	slotRootParameter[1].InitAsConstantBufferView(1, 0U, D3D12_SHADER_VISIBILITY_PIXEL);	//Per light CB

	//g buffer textures
	CD3DX12_DESCRIPTOR_RANGE ranges[GBUFFER_NUM + 1];

	for (int i = 0; i < GBUFFER_NUM + 1; ++i)
	{
		ranges[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)1, i);

		slotRootParameter[i + 2].InitAsDescriptorTable(1, &ranges[i], D3D12_SHADER_VISIBILITY_PIXEL);
	}

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init((UINT)_countof(slotRootParameter), slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> pSignature;
	ComPtr<ID3DBlob> pError;

	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pSignature.GetAddressOf(), pError.GetAddressOf());

	if (FAILED(hr))
	{
		if (pError != nullptr)
		{
			OutputDebugStringA((char*)pError->GetBufferPointer());
		}

		LOG_ERROR(tag, L"Failed to create light pass serialize root signature!");

		return false;
	}

	hr = m_pDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(m_pLightSignature.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create light pass root signature!");

		return false;
	}

	return true;
}

bool BasicApp::CreatePostProcessingRootSignature()
{
	//+1 for rendered screen texture
	CD3DX12_ROOT_PARAMETER slotRootParameter[2] = {};

	slotRootParameter[0].InitAsConstantBufferView(0, 0U, D3D12_SHADER_VISIBILITY_PIXEL);	//Per frame CB

	CD3DX12_DESCRIPTOR_RANGE range;
	range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, (UINT)1, 0);

	slotRootParameter[1].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init((UINT)_countof(slotRootParameter), slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> pSignature;
	ComPtr<ID3DBlob> pError;

	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pSignature.GetAddressOf(), pError.GetAddressOf());

	if (FAILED(hr))
	{
		if (pError != nullptr)
		{
			OutputDebugStringA((char*)pError->GetBufferPointer());
		}

		LOG_ERROR(tag, L"Failed to create post processing serialize root signature!");

		return false;
	}

	hr = m_pDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(m_pPostProcessingSignature.GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create post processing root signature!");

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
	//+1 for depth buffer, +1 for post processing rtv
	srvDesc.NumDescriptors = (UINT)TextureManager::GetInstance()->GetTextures()->size() + GBUFFER_NUM + 2;
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
		//Create shader resource view
		srvDesc.Texture2D.MipLevels = it->second->Resource->GetDesc().MipLevels;
		srvDesc.Format = it->second->Resource->GetDesc().Format;

		m_pDevice->CreateShaderResourceView(it->second->Resource.Get(), &srvDesc, descHandle);

		//Offset handle ready for next view
		descHandle.Offset(1, m_uiCBVSRVDescSize);

		it->second->HeapIndex = uiCount;

		++uiCount;
	}

	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Format = m_BackBufferFormat;

	//Create SRV for g buffer textures
	m_pDevice->CreateShaderResourceView(m_GBuffer.m_pAlbedo.Get(), &srvDesc, descHandle);
	descHandle.Offset(1, m_uiCBVSRVDescSize);

	m_pDevice->CreateShaderResourceView(m_GBuffer.m_pNormal.Get(), &srvDesc, descHandle);
	descHandle.Offset(1, m_uiCBVSRVDescSize);

	m_pDevice->CreateShaderResourceView(m_GBuffer.m_pTangent.Get(), &srvDesc, descHandle);
	descHandle.Offset(1, m_uiCBVSRVDescSize);

	m_pDevice->CreateShaderResourceView(m_GBuffer.m_pDiffuse.Get(), &srvDesc, descHandle);
	descHandle.Offset(1, m_uiCBVSRVDescSize);

	m_pDevice->CreateShaderResourceView(m_GBuffer.m_pSpecular.Get(), &srvDesc, descHandle);
	descHandle.Offset(1, m_uiCBVSRVDescSize);

	m_pDevice->CreateShaderResourceView(m_GBuffer.m_pAmbient.Get(), &srvDesc, descHandle);
	descHandle.Offset(1, m_uiCBVSRVDescSize);

	m_pDevice->CreateShaderResourceView(m_GBuffer.m_pShadow.Get(), &srvDesc, descHandle);
	descHandle.Offset(1, m_uiCBVSRVDescSize);

	//Create srv for depth buffer
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

	m_pDevice->CreateShaderResourceView(m_pDepthStencilBuffer.Get(), &srvDesc, descHandle);
	descHandle.Offset(1, m_uiCBVSRVDescSize);

	//Create srv for post processing rtv
	srvDesc.Format = m_BackBufferFormat;

	m_pDevice->CreateShaderResourceView(m_pPostProcessingRTV.Get(), &srvDesc, descHandle);
}

bool BasicApp::CreatePSOs()
{
	if (CreateGBufferPSO() == false)
	{
		return false;
	}

	if (CreateLightPassPSO() == false)
	{
		return false;
	}

	if (CreatePostProcessingPSO() == false)
	{
		return false;
	}

	//Create back buffer PSO's
	//for (std::unordered_map<std::string, void*>::iterator itA = ShaderManager::GetInstance()->GetShaders()->begin(); itA != ShaderManager::GetInstance()->GetShaders()->end(); ++itA)
	//{
	//	if (static_cast<Shader<VisibleGameObjectCB>*>(itA->second)->GetShaderType() != ShaderType::Vertex)
	//	{
	//		continue;
	//	}

	//	for (std::unordered_map<std::string, void*>::iterator itB = ShaderManager::GetInstance()->GetShaders()->begin(); itB != ShaderManager::GetInstance()->GetShaders()->end(); ++itB)
	//	{
	//		if (static_cast<Shader<VisibleGameObjectCB>*>(itB->second)->GetShaderType() != ShaderType::Pixel)
	//		{
	//			continue;
	//		}

	//		psoDesc.VS = CD3DX12_SHADER_BYTECODE(ShaderManager::GetInstance()->GetShader<VisibleGameObjectCB>(itA->first)->GetShaderBlob().Get());
	//		psoDesc.PS = CD3DX12_SHADER_BYTECODE(ShaderManager::GetInstance()->GetShader<VisibleGameObjectCB>(itB->first)->GetShaderBlob().Get());

	//		ppPipelineState = new Microsoft::WRL::ComPtr<ID3D12PipelineState>();

	//		hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(ppPipelineState->GetAddressOf()));

	//		if (FAILED(hr))
	//		{
	//			LOG_ERROR(tag, L"Failed to create the pipeline state object!");

	//			return false;
	//		}

	//		psoDescription.VSName = itA->first;
	//		psoDescription.PSName = itB->first;

	//		m_PipelineStates[psoDescription] = ppPipelineState->Get();
	//	}
	//}

	//PSODesc desc = { "VS", "PS_ParallaxOcclusion" };

	//m_pPipelineState = m_PipelineStates[desc].Get();

	return true;
}

bool BasicApp::CreateGBufferPSO()
{
	//Initialise constant pipeline desc propertires
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { m_GBufferVertexInputLayoutDesc.data(), (UINT)m_GBufferVertexInputLayoutDesc.size() };
	psoDesc.pRootSignature = m_pGBufferSignature.Get();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = GBUFFER_NUM;
	psoDesc.SampleDesc.Count = m_b4xMSAAState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m_b4xMSAAState ? (m_uiMSAAQuality - 1) : 0;
	psoDesc.DSVFormat = m_DepthStencilFormat;

	for (int i = 0; i < GBUFFER_NUM; ++i)
	{
		psoDesc.RTVFormats[i] = m_BackBufferFormat;
	}

	std::string psNames[] =
	{
		"PS_GBuffer_Nothing",
		"PS_GBuffer_Normal",
		"PS_GBuffer_Parallax",
		"PS_GBuffer_ParallaxOcclusion",
		"PS_GBuffer_ParallaxShadow"
	};

	PSODesc psoDescription;

	for (int i = 0; i < _countof(psNames); ++i)
	{
		Microsoft::WRL::ComPtr<ID3D12PipelineState>* ppPipelineState = new Microsoft::WRL::ComPtr<ID3D12PipelineState>();

		psoDesc.VS = CD3DX12_SHADER_BYTECODE(ShaderManager::GetInstance()->GetShader<VisibleGameObjectCB>("VS_GBuffer")->GetShaderBlob().Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(ShaderManager::GetInstance()->GetShader<VisibleGameObjectCB>(psNames[i])->GetShaderBlob().Get());

		HRESULT hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(ppPipelineState->GetAddressOf()));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to create the GBuffer pipeline state object!");

			return false;
		}

		psoDescription.VSName = "VS_GBuffer";
		psoDescription.PSName = psNames[i];

		m_PipelineStates[psoDescription] = ppPipelineState->Get();
	}

	return true;
}

bool BasicApp::CreateLightPassPSO()
{
	//Create blend state so it accumulates rather than overwrites
	D3D12_RENDER_TARGET_BLEND_DESC blendState;
	blendState.BlendEnable = TRUE;
	blendState.LogicOpEnable = FALSE;
	blendState.SrcBlend = D3D12_BLEND_ONE;
	blendState.DestBlend = D3D12_BLEND_ONE;
	blendState.BlendOp = D3D12_BLEND_OP_ADD;
	blendState.SrcBlendAlpha = D3D12_BLEND_ONE;
	blendState.DestBlendAlpha = D3D12_BLEND_ZERO;
	blendState.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendState.LogicOp = D3D12_LOGIC_OP_NOOP;
	blendState.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//Initialise constant pipeline desc properties
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { m_ScreenQuadVertexInputLayoutDesc.data(), (UINT)m_ScreenQuadVertexInputLayoutDesc.size() };
	psoDesc.pRootSignature = m_pLightSignature.Get();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState.RenderTarget[0] = blendState;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_BackBufferFormat;
	psoDesc.SampleDesc.Count = m_b4xMSAAState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m_b4xMSAAState ? (m_uiMSAAQuality - 1) : 0;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

	std::string psNames[] =
	{
		"PS_LightPass_Nothing",
		"PS_LightPass_ParallaxShadow"
	};

	PSODesc psoDescription;

	for (int i = 0; i < _countof(psNames); ++i)
	{
		Microsoft::WRL::ComPtr<ID3D12PipelineState>* ppPipelineState = new Microsoft::WRL::ComPtr<ID3D12PipelineState>();

		psoDesc.VS = CD3DX12_SHADER_BYTECODE(ShaderManager::GetInstance()->GetShader<VisibleGameObjectCB>("VS_ScreenQuad")->GetShaderBlob().Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(ShaderManager::GetInstance()->GetShader<VisibleGameObjectCB>(psNames[i])->GetShaderBlob().Get());

		HRESULT hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(ppPipelineState->GetAddressOf()));

		if (FAILED(hr))
		{
			LOG_ERROR(tag, L"Failed to create the light pass pipeline state object!");

			return false;
		}

		psoDescription.VSName = "VS_ScreenQuad";
		psoDescription.PSName = psNames[i];

		m_PipelineStates[psoDescription] = ppPipelineState->Get();
	}


	return true;
}

bool BasicApp::CreatePostProcessingPSO()
{
	//Initialise constant pipeline desc properties
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { m_ScreenQuadVertexInputLayoutDesc.data(), (UINT)m_ScreenQuadVertexInputLayoutDesc.size() };
	psoDesc.pRootSignature = m_pPostProcessingSignature.Get();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_BackBufferFormat;
	psoDesc.SampleDesc.Count = m_b4xMSAAState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m_b4xMSAAState ? (m_uiMSAAQuality - 1) : 0;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

	Microsoft::WRL::ComPtr<ID3D12PipelineState>* ppPipelineState = new Microsoft::WRL::ComPtr<ID3D12PipelineState>();

	PSODesc psoDescription;

	//Create post processing PSO
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(ShaderManager::GetInstance()->GetShader<VisibleGameObjectCB>("VS_ScreenQuad")->GetShaderBlob().Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(ShaderManager::GetInstance()->GetShader<VisibleGameObjectCB>("PS_PostProcessing")->GetShaderBlob().Get());

	HRESULT hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(ppPipelineState->GetAddressOf()));

	if (FAILED(hr))
	{
		LOG_ERROR(tag, L"Failed to create the post processing pipeline state object!");

		return false;
	}

	psoDescription.VSName = "VS_ScreenQuad";
	psoDescription.PSName = "PS_PostProcessing";

	m_PipelineStates[psoDescription] = ppPipelineState->Get();

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

void BasicApp::PopulateGBuffer()
{
	PIX_ONLY(PIXBeginEvent(m_pGraphicsCommandList.Get(), PIX_COLOR(50, 50, 50), "Populate G buffer"));

	PSODesc psoDesc = { "VS_GBuffer", m_sCurrentGBufferPSName };

	m_pGraphicsCommandList->SetPipelineState(m_PipelineStates[psoDesc].Get());

	m_pGraphicsCommandList->SetGraphicsRootSignature(m_pGBufferSignature.Get());
	m_pGraphicsCommandList->RSSetViewports(1, &m_Viewport);
	m_pGraphicsCommandList->RSSetScissorRects(1, &m_ScissorRect);

	const float GBufferClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	CD3DX12_RESOURCE_BARRIER* pResourceBarriers = new CD3DX12_RESOURCE_BARRIER[GBUFFER_NUM];
	pResourceBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBuffer.m_pAlbedo.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pResourceBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBuffer.m_pNormal.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pResourceBarriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBuffer.m_pTangent.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pResourceBarriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBuffer.m_pDiffuse.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pResourceBarriers[4] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBuffer.m_pSpecular.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pResourceBarriers[5] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBuffer.m_pAmbient.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pResourceBarriers[6] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBuffer.m_pShadow.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

	m_pGraphicsCommandList->ResourceBarrier(GBUFFER_NUM, pResourceBarriers);

	delete[] pResourceBarriers;

	//Clear g buffer
	for (int i = 0; i < GBUFFER_NUM; ++i)
	{
		m_pGraphicsCommandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), s_kuiSwapChainBufferCount + i, m_uiRTVDescSize), GBufferClearColor, 0, nullptr);
	}

	//Clear depth stencil view
	m_pGraphicsCommandList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	m_pGraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//Set the per frame constant buffer
	m_pGraphicsCommandList->SetGraphicsRootConstantBufferView(0, m_pGBufferPerFrameCB->Get()->GetGPUVirtualAddress());

	std::unordered_map<std::string, GameObject*>* pGameObjects = ObjectManager::GetInstance()->GetGameObjects();

	D3D12_CPU_DESCRIPTOR_HANDLE* pRenderTargetDescs = new D3D12_CPU_DESCRIPTOR_HANDLE[GBUFFER_NUM];

	for (int i = 0; i < GBUFFER_NUM; ++i)
	{
		pRenderTargetDescs[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), s_kuiSwapChainBufferCount + i, m_uiRTVDescSize);
	}

	// Bind the depth buffer
	m_pGraphicsCommandList->OMSetRenderTargets(GBUFFER_NUM, pRenderTargetDescs, FALSE, &GetDepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { m_pTextureDescHeap.Get() };
	m_pGraphicsCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	//Draw the gameobjects
	UINT uiPerObjByteSize = DirectXHelper::CalculatePaddedConstantBufferSize(sizeof(VisibleGameObjectCB));
	D3D12_GPU_VIRTUAL_ADDRESS perObjCBAddress = ShaderManager::GetInstance()->GetShaderConstantUploadBuffer<VisibleGameObjectCB>("VS")->Get()->GetGPUVirtualAddress();

	UINT uiMatByteSize = DirectXHelper::CalculatePaddedConstantBufferSize(sizeof(MaterialCB));
	D3D12_GPU_VIRTUAL_ADDRESS matCBAddress;

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

			matCBAddress = m_pMaterialCB->Get()->GetGPUVirtualAddress() + uiMatByteSize * pVisibleGameObject->GetMaterial()->CBIndex;

			//Set the material constant buffer
			m_pGraphicsCommandList->SetGraphicsRootConstantBufferView(2, matCBAddress);

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

	//transistion g buffer to read state
	pResourceBarriers = new CD3DX12_RESOURCE_BARRIER[GBUFFER_NUM];
	pResourceBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBuffer.m_pAlbedo.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	pResourceBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBuffer.m_pNormal.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	pResourceBarriers[2] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBuffer.m_pTangent.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	pResourceBarriers[3] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBuffer.m_pDiffuse.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	pResourceBarriers[4] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBuffer.m_pSpecular.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	pResourceBarriers[5] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBuffer.m_pAmbient.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	pResourceBarriers[6] = CD3DX12_RESOURCE_BARRIER::Transition(m_GBuffer.m_pShadow.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);

	m_pGraphicsCommandList->ResourceBarrier(GBUFFER_NUM, pResourceBarriers);

	PIX_ONLY(PIXEndEvent(m_pGraphicsCommandList.Get()));
}

void BasicApp::DoLightPass()
{
	PIX_ONLY(PIXBeginEvent(m_pGraphicsCommandList.Get(), PIX_COLOR(50, 50, 50), "Render to texture"));

	PSODesc lightPassPSODesc = { "VS_ScreenQuad", m_sCurrentLightPassPSName };

	m_pGraphicsCommandList->SetPipelineState(m_PipelineStates[lightPassPSODesc].Get());

	m_pGraphicsCommandList->SetGraphicsRootSignature(m_pLightSignature.Get());

	const float RTVClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };

	//Transition resources
	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pPostProcessingRTV.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_DEPTH_READ));

	m_pGraphicsCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	m_pGraphicsCommandList->OMSetRenderTargets(1, &CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), s_kuiSwapChainBufferCount + GBUFFER_NUM, m_uiRTVDescSize), FALSE, nullptr);

	m_pGraphicsCommandList->ClearRenderTargetView(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), s_kuiSwapChainBufferCount + GBUFFER_NUM, m_uiRTVDescSize), RTVClearColor, 0, nullptr);

	m_pGraphicsCommandList->IASetVertexBuffers(0, 1, &m_ScreenQuadVBView);

	//Bind g buffer and depth stencil textures
	CD3DX12_GPU_DESCRIPTOR_HANDLE textureAddress;

	textureAddress = m_pTextureDescHeap->GetGPUDescriptorHandleForHeapStart();
	textureAddress.Offset(TextureManager::GetInstance()->GetTextures()->size(), m_uiCBVSRVDescSize);

	for (int i = 0; i < GBUFFER_NUM + 1; ++i)
	{
		m_pGraphicsCommandList->SetGraphicsRootDescriptorTable(i + 2, textureAddress);
		textureAddress.Offset(1, m_uiCBVSRVDescSize);
	}

	//Bind the light pass constant buffers
	m_pGraphicsCommandList->SetGraphicsRootConstantBufferView(0, m_pLightPassPerFrameCB->Get()->GetGPUVirtualAddress());

	UINT uiLightByteSize = DirectXHelper::CalculatePaddedConstantBufferSize(sizeof(LightCB));
	D3D12_GPU_VIRTUAL_ADDRESS lightAddress;

	//Draw once for each enabled light
	for (std::unordered_map<std::string, Light*>::iterator it = LightManager::GetInstance()->GetLights()->begin(); it != LightManager::GetInstance()->GetLights()->end(); ++it)
	{
		if (it->second->Enabled == true)
		{
			//Set light constant buffer
			lightAddress = LightManager::GetInstance()->GetUploadBuffer()->Get()->GetGPUVirtualAddress() + uiLightByteSize * it->second->Index;
			m_pGraphicsCommandList->SetGraphicsRootConstantBufferView(1, lightAddress);

			//Draw for each enabled light
			m_pGraphicsCommandList->DrawInstanced(4, 1, 0, 0);
		}
	}

	//Transition resources back
	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pPostProcessingRTV.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	PIX_ONLY(PIXEndEvent(m_pGraphicsCommandList.Get()));
}

void BasicApp::DoPostProcessing()
{
	PIX_ONLY(PIXBeginEvent(m_pGraphicsCommandList.Get(), PIX_COLOR(50, 50, 50), "Carry out post processing"));

	PSODesc postProcessingPSODesc = { "VS_ScreenQuad", "PS_PostProcessing" };

	m_pGraphicsCommandList->SetPipelineState(m_PipelineStates[postProcessingPSODesc].Get());

	m_pGraphicsCommandList->SetGraphicsRootSignature(m_pPostProcessingSignature.Get());

	const float RTVClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	//Transition resources
	m_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_pGraphicsCommandList->OMSetRenderTargets(1, &GetCurrentBackBufferView(), FALSE, nullptr);

	m_pGraphicsCommandList->ClearRenderTargetView(GetCurrentBackBufferView(), RTVClearColor, 0, nullptr);

	//Bind rendered frame
	CD3DX12_GPU_DESCRIPTOR_HANDLE textureAddress;

	textureAddress = m_pTextureDescHeap->GetGPUDescriptorHandleForHeapStart();
	textureAddress.Offset(TextureManager::GetInstance()->GetTextures()->size() + GBUFFER_NUM + 1, m_uiCBVSRVDescSize);

	m_pGraphicsCommandList->SetGraphicsRootDescriptorTable(1, textureAddress);

	//Bind the post processing constant buffers
	m_pGraphicsCommandList->SetGraphicsRootConstantBufferView(0, m_pPostProcessingPerFrameCB->Get()->GetGPUVirtualAddress());

	m_pGraphicsCommandList->DrawInstanced(4, 1, 0, 0);

	PIX_ONLY(PIXEndEvent(m_pGraphicsCommandList.Get()));
}

void BasicApp::UpdatePostProcessingCB()
{
	PostProcessingPerFrameCB postProcessingPerFrameCB;
	postProcessingPerFrameCB.ScreenWidth = (int)WindowManager::GetInstance()->GetWindowWidth();
	postProcessingPerFrameCB.ScreenHeight = (int)WindowManager::GetInstance()->GetWindowHeight();
	postProcessingPerFrameCB.Enabled = (int)m_bEnableBoxBlur;
	postProcessingPerFrameCB.BoxBlurNumber = m_iBoxBlurLevel;

	if (m_pPostProcessingPerFrameCB != nullptr)
	{
		m_pPostProcessingPerFrameCB->CopyData(0, postProcessingPerFrameCB);
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
		pBasicApp->m_sCurrentGBufferPSName = "PS_GBuffer_Nothing";
		pBasicApp->m_sCurrentLightPassPSName = "PS_LightPass_Nothing";
		break;

	case 49: //1
		pBasicApp->m_sCurrentGBufferPSName = "PS_GBuffer_Normal";
		pBasicApp->m_sCurrentLightPassPSName = "PS_LightPass_Nothing";
		break;

	case 50: //2
		pBasicApp->m_sCurrentGBufferPSName = "PS_GBuffer_Parallax";
		pBasicApp->m_sCurrentLightPassPSName = "PS_LightPass_Nothing";
		break;

	case 51: //3
		pBasicApp->m_sCurrentGBufferPSName = "PS_GBuffer_ParallaxOcclusion";
		pBasicApp->m_sCurrentLightPassPSName = "PS_LightPass_Nothing";
		break;

	case 52: //4
		pBasicApp->m_sCurrentGBufferPSName = "PS_GBuffer_ParallaxShadow";
		pBasicApp->m_sCurrentLightPassPSName = "PS_LightPass_ParallaxShadow";
		break;

	case 53: //5
		LightManager::GetInstance()->ToggleLight("point");
		break;

	case 54: //6
		LightManager::GetInstance()->ToggleLight("spot");
		break;

	case 55: //7
		LightManager::GetInstance()->ToggleLight("direction");
		break;

	case 56: //8
		pBasicApp->m_iBoxBlurLevel += 1;
		pBasicApp->UpdatePostProcessingCB();
		break;

	case 57: //9
		if (pBasicApp->m_iBoxBlurLevel - 1 > 0)
		{
			pBasicApp->m_iBoxBlurLevel -= 1;
			pBasicApp->UpdatePostProcessingCB();
		}
		break;

	case 69:	//e
		pBasicApp->m_bEnableBoxBlur = !pBasicApp->m_bEnableBoxBlur;
		pBasicApp->UpdatePostProcessingCB();
		break;

	case 81: // q
		pBasicApp->m_bRotateCube = !pBasicApp->m_bRotateCube;
		break;
	}
}

float fMoveSens = 10.0f;

void BasicApp::OnKeyHeld(void* pObject, int iKeycode, const Timer& ktimer)
{
	GameObject* pGameobject = ObjectManager::GetInstance()->GetGameObject("Box1");

	XMFLOAT4 right = pGameobject->GetRightVector();
	XMFLOAT4 up = pGameobject->GetUpVector();

	XMFLOAT3 translation;

	Light* pLight = LightManager::GetInstance()->GetLight("point");

	switch (iKeycode)
	{
	case VK_RIGHT:
		XMStoreFloat3(&translation, XMVectorSet(1, 0, 0, 0) * fMoveSens * ktimer.DeltaTime());
		pGameobject->Translate(translation);
		break;
	case VK_LEFT:
		XMStoreFloat3(&translation, XMVectorSet(-1, 0, 0, 0) * fMoveSens * ktimer.DeltaTime());
		pGameobject->Translate(translation);
		break;
	case VK_UP:
		XMStoreFloat3(&translation, XMVectorSet(0, 1, 0, 0) * fMoveSens * ktimer.DeltaTime());
		pGameobject->Translate(translation);
		break;
	case VK_DOWN:
		XMStoreFloat3(&translation, XMVectorSet(0, -1, 0, 0) * fMoveSens * ktimer.DeltaTime());
		pGameobject->Translate(translation);
		break;

	case 73: //i
		XMStoreFloat3(&pLight->lightCB.Position, XMLoadFloat3(&pLight->lightCB.Position) + XMVectorSet(0, 1, 0, 0) * fMoveSens * ktimer.DeltaTime());
		LightManager::GetInstance()->UpdateUploadBuffer("point");
		break;

	case 74: //j
		XMStoreFloat3(&pLight->lightCB.Position, XMLoadFloat3(&pLight->lightCB.Position) + XMVectorSet(-1, 0, 0, 0) * fMoveSens * ktimer.DeltaTime());
		LightManager::GetInstance()->UpdateUploadBuffer("point");
		break;

	case 75: //k
		XMStoreFloat3(&pLight->lightCB.Position, XMLoadFloat3(&pLight->lightCB.Position) + XMVectorSet(0, -1, 0, 0) * fMoveSens * ktimer.DeltaTime());
		LightManager::GetInstance()->UpdateUploadBuffer("point");
		break;

	case 76: //l
		XMStoreFloat3(&pLight->lightCB.Position, XMLoadFloat3(&pLight->lightCB.Position) + XMVectorSet(1, 0, 0, 0) * fMoveSens * ktimer.DeltaTime());
		LightManager::GetInstance()->UpdateUploadBuffer("point");
		break;
	}
}
