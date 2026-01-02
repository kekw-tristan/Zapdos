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
    std::cout << "Initialize\n";

    m_pTimer = new cTimer();
    m_pTimer->Start();

    m_pWindow = new cWindow();
    m_pWindow->Initialize(L"Zapdos", L"gameWindow", 1280, 720, m_pTimer);

    m_pDirectX12 = new cDirectX12();
    m_pDirectX12->Initialize(m_pWindow, m_pTimer, c_numberOfRenderItems, 1000);

    m_pScene = std::make_unique<cScene>();

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

    std::vector<cMeshGenerator::sMeshData> meshes;
    std::vector<XMMATRIX> worldMatrices;

    std::string path = "..\\Assets\\Objects\\scene.gltf";
    meshGenerator.LoadModelFromGLTF(path, meshes, worldMatrices);

    for (int i = 0; i < meshes.size(); ++i)
    {
        std::string name = "scene_" + std::to_string(i);
        m_pDirectX12->InitializeMesh(meshes[i], name, XMFLOAT4(0.4f, 0.8f, 0.4f, 1.f));
    }

    sMeshGeometry* pMeshGeo = m_pDirectX12->InitializeGeometryBuffer();

    static sMaterial groundMaterial;
    groundMaterial.albedo = XMFLOAT3(0.7f, 0.7f, 0.7f);
    groundMaterial.specularExponent = 16.0f;

    for (int i = 0; i < meshes.size(); ++i)
    {
        sRenderItem ri;
        std::string name = "scene_" + std::to_string(i);

        ri.pGeometry = pMeshGeo;
        ri.pMaterial = &groundMaterial;
        ri.objCBIndex = static_cast<UINT>(i);

        const auto& submesh = pMeshGeo->drawArguments.at(name);
        ri.indexCount = submesh.indexCount;
        ri.startIndexLocation = submesh.startIndexLocation;
        ri.baseVertexLocation = submesh.startVertexLocation;

        XMStoreFloat4x4(&ri.worldMatrix, worldMatrices[i]);

        m_pScene->GetRenderItems().emplace_back(std::move(ri));
    }

    std::cout << "Meshes initialized\n";
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::InitializeLights()
{
    auto& lights = m_pScene->GetLight();

    sLightConstants directionalLight{};
    directionalLight.strength = XMFLOAT3(1, 1, 1);
    directionalLight.direction = XMFLOAT3(0, -1, 0);
    directionalLight.type = 0;

    lights.push_back(directionalLight);

    const int pointLightCount = 4;
    const float radius = 30.0f;
    const float yHeight = 20.0f;

    for (int i = 0; i < pointLightCount; ++i)
    {
        sLightConstants light{};
        float angle = XM_2PI * i / pointLightCount;

        light.position = XMFLOAT3(
            radius * cosf(angle),
            yHeight,
            radius * sinf(angle)
        );

        light.strength = XMFLOAT3(1, 1, 1);
        light.falloffStart = 10.0f;
        light.falloffEnd = 50.0f;
        light.type = 1;

        lights.push_back(light);
    }
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Update()
{
    HandleInput();

    XMVECTOR pos = XMLoadFloat3(&m_position);
    XMMATRIX rot = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);

    XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rot);
    XMVECTOR up = XMVector3TransformCoord(XMVectorSet(0, 1, 0, 0), rot);

    XMMATRIX view = XMMatrixLookAtLH(pos, pos + forward, up);
    XMStoreFloat4x4(&m_view, view);

    XMFLOAT3 camPos;
    XMStoreFloat3(&camPos, pos);

    m_pDirectX12->Update(
        view,
        camPos,
        &m_pScene->GetRenderItems(),
        &m_pScene->GetLight()
    );
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::HandleInput()
{
    float speed = 0.1f;
    constexpr float rotSpeed = XMConvertToRadians(1.0f);

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


