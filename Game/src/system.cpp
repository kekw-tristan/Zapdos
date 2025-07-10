#include "system.h"

#include <algorithm>
#include <iostream>
#include <DirectXColors.h>
#include <wrl.h>
#include <d3d12.h>

#include "core/window.h"
#include "core/timer.h"
#include "core/input.h"

#include "graphics/directx12.h"
#include "graphics/directx12Util.h"
#include "Graphics/renderItem.h"
#include "graphics/vertex.h"
#include "graphics/meshGeometry.h"
#include "graphics/meshGenerator.h"

using namespace DirectX;
using namespace Microsoft::WRL;

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Initialize()
{
	std::cout << "Initialize" << "\n";
	m_pTimer = new cTimer();
	m_pTimer->Start();
	
	m_pWindow = new cWindow();
	m_pWindow->Initialize(L"Zapdos", L"gameWindow", 1280, 720, m_pTimer);

	m_pDirectX12 = new cDirectX12();
	m_pDirectX12->Initialize(m_pWindow, m_pTimer, c_numberOfRenderItems, 1000);

    InitializeRenderItems(); 
    InitializeLights();
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Run()
{
	while (m_pWindow->GetIsRunning())
	{
		m_pWindow->MessageHandling();

		if (m_pWindow->GetIsWindowPaused())
			continue;

		m_pTimer->Tick();

		m_pDirectX12->CalculateFrameStats();
		Update();
		m_pDirectX12->Draw();
	}	
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Finalize()
{
	std::cout << "Finalize" << "\n";

	m_pDirectX12->Finalize();

	delete m_pWindow;
	delete m_pDirectX12;
	delete m_pTimer;
}

// --------------------------------------------------------------------------------------------------------------------------
void cSystem::InitializeRenderItems()
{
    cMeshGenerator meshGenerator; 

    cMeshGenerator::sMeshData meshData; 

    std::string path = "C:\\Users\\Tristan\\source\\repos\\Zapdos\\Assets\\Objects\\Untitled.gltf";
    meshGenerator.LoadModelFromGLTF(path, meshData);

    for (auto value : meshData.indices16)
        std::cout << value << "\n";

    for (auto v : meshData.vertices)
    {
        std::cout << v.position.x << ", " << v.position.y << ", " << v.position.z << "\n";
    }

    
    m_pDirectX12->InitializeMesh(meshData, std::string("cube"), XMFLOAT4(0.4f, 0.8f, 0.4f, 1.f));

    sMeshGeometry* pMeshGeo = m_pDirectX12->InitializeGeometryBuffer();

    static sMaterial groundMaterial;
    groundMaterial.albedo = XMFLOAT3(0.4f, 0.8f, 0.4f);
    groundMaterial.specularExponent = 16.0f;

    sRenderItem renderItem;
    renderItem.pGeometry = pMeshGeo;
    renderItem.pMaterial = &groundMaterial;
    renderItem.objCBIndex = static_cast<UINT>(m_renderItems.size());

    const auto& submesh = pMeshGeo->drawArguments.at("cube");
    renderItem.indexCount = submesh.indexCount;
    renderItem.startIndexLocation = submesh.startIndexLocation;
    renderItem.baseVertexLocation = submesh.startVertexLocation;

    m_renderItems.emplace_back(std::move(renderItem));

    XMMATRIX trunkScale = XMMatrixScaling(3.f,   3.f, 3.f);
    XMMATRIX trunkTranslate = XMMatrixTranslation(3, 0.25,2);
    XMStoreFloat4x4(&renderItem.worldMatrix, trunkScale * trunkTranslate);

    std::cout << "mesh initialized\n";
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::InitializeLights()
{
    sLightConstants directionalLight = {};

    // Strength (RGB Intensity)
    directionalLight.strength = XMFLOAT3(1.0f, 1.0f, 1.0f); // white light

    // FalloffStart and FalloffEnd irrelevant for directional, but initialise safely
    directionalLight.falloffStart = 1.0f;
    directionalLight.falloffEnd = 10.0f;

    // Direction (pointing downwards and slightly forward for example)
    directionalLight.direction = XMFLOAT3(0.0f, -1.0f, 0.0f);

    // Position irrelevant for directional, initialise to zero
    directionalLight.position = XMFLOAT3(0.0f, 0.0f, 0.0f);

    // SpotPower irrelevant for directional, initialise to zero
    directionalLight.spotPower = 0.0f;

    // Type: assuming 0 = directional, 1 = point, 2 = spot (define per your engine)
    directionalLight.type = 0;

    // Padding (if used for 16-byte alignment)
    directionalLight.padding = XMFLOAT3(0.0f, 0.0f, 0.0f);

    m_lights.push_back(directionalLight);
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Update()
{
    HandleInput();

    XMVECTOR pos = XMLoadFloat3(&m_position);
    XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);

    XMVECTOR forward    = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rot);
    XMVECTOR up         = XMVector3TransformCoord(XMVectorSet(0, 1, 0, 0), rot);
    XMVECTOR at         = pos + forward;

    XMMATRIX view = XMMatrixLookAtLH(pos, at, up);
    XMStoreFloat4x4(&m_view, view);

    XMFLOAT3 camPos;
    XMStoreFloat3(&camPos, pos);

    m_pDirectX12->Update(view, camPos, &m_renderItems, &m_lights);
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::HandleInput()
{
    float speed = 0.1f;
    constexpr float rotSpeed = XMConvertToRadians(1.0f);

    // Create rotation matrix from yaw and pitch to compute directions
    XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);

    XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rot);
    XMVECTOR right = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), rot);

    XMVECTOR pos = XMLoadFloat3(&m_position);

    // WASD movement
    if (cInput::IsKeyDown('W'))
        pos += speed * forward;

    if (cInput::IsKeyDown('S'))
        pos -= speed * forward;

    if (cInput::IsKeyDown('A'))
        pos -= speed * right;

    if (cInput::IsKeyDown('D'))
        pos += speed * right;

    // Arrow key rotation
    if (cInput::IsKeyDown(VK_LEFT))
        m_yaw -= rotSpeed;

    if (cInput::IsKeyDown(VK_RIGHT))
        m_yaw += rotSpeed;

    if (cInput::IsKeyDown(VK_DOWN))
        m_pitch += rotSpeed;

    if (cInput::IsKeyDown(VK_UP))
        m_pitch -= rotSpeed;

    // Clamp pitch to avoid gimbal lock
    m_pitch = std::clamp(m_pitch, -XM_PIDIV2 + 0.1f, XM_PIDIV2 - 0.1f);

    XMStoreFloat3(&m_position, pos);
}

// --------------------------------------------------------------------------------------------------------------------------


