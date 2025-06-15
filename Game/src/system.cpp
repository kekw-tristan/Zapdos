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
	m_pDirectX12->Initialize(m_pWindow, m_pTimer, c_numberOfRenderItems);

	InitializeRenderItems();
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Run()
{
	while (m_pWindow->GetIsRunning())
	{
		m_pWindow->MessageHandling();

		// check if paused		
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

	delete m_pWindow;

	m_pDirectX12->Finalize();
	delete m_pDirectX12;
	delete m_pTimer;
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::InitializeRenderItems()
{
    cMeshGenerator meshGenerator;

    // Initialize mesh shapes
    m_pDirectX12->InitializeMesh(meshGenerator.CreateCube(), std::string("cube"), XMFLOAT4(0.4f, 0.8f, 0.4f, 1.f));       // green ground
    m_pDirectX12->InitializeMesh(meshGenerator.CreateSphere(1.0f, 20, 20), std::string("sphere"), XMFLOAT4(0.1f, 0.6f, 0.1f, 1.f)); // foliage
    m_pDirectX12->InitializeMesh(meshGenerator.CreateCylinder(0.3, 0.3, 1.2f, 20, 20), std::string("cylinder"), XMFLOAT4(0.55f, 0.27f, 0.07f, 1.f)); // trunk

    sMeshGeometry* pMeshGeo = m_pDirectX12->InitializeGeometryBuffer();

    // Create example materials
    static sMaterial groundMaterial;
    groundMaterial.albedo = XMFLOAT3(0.4f, 0.8f, 0.4f);
    groundMaterial.specularExponent = 16.0f;

    static sMaterial trunkMaterial;
    trunkMaterial.albedo = XMFLOAT3(0.55f, 0.27f, 0.07f);
    trunkMaterial.specularExponent = 32.0f;

    static sMaterial foliageMaterial;
    foliageMaterial.albedo = XMFLOAT3(0.1f, 0.6f, 0.1f);
    foliageMaterial.specularExponent = 8.0f;

    const int groundRows = 20;
    const int groundCols = 20;
    const float spacing = 5.0f;

    m_renderItems.clear();
    m_renderItems.reserve(groundRows * groundCols + 5000);
    std::cout << groundRows * groundCols + 5000 << std::endl;

    // Generate ground + trees
    for (int z = 0; z < groundRows; ++z)
    {
        for (int x = 0; x < groundCols; ++x)
        {
            // Ground block
            sRenderItem cubeItem;
            cubeItem.pGeometry = pMeshGeo;
            cubeItem.pMaterial = &groundMaterial;       // Assign ground material
            cubeItem.objCBIndex = static_cast<UINT>(m_renderItems.size());

            const auto& submesh = pMeshGeo->drawArguments.at("cube");
            cubeItem.indexCount = submesh.indexCount;
            cubeItem.startIndexLocation = submesh.startIndexLocation;
            cubeItem.baseVertexLocation = submesh.startVertexLocation;

            XMMATRIX scale = XMMatrixScaling(5.0f, 0.5f, 5.0f);
            XMMATRIX translate = XMMatrixTranslation(x * spacing, -0.25f, z * spacing);
            XMStoreFloat4x4(&cubeItem.worldMatrix, scale * translate);
            m_renderItems.emplace_back(std::move(cubeItem));

            // Place trees densely, e.g. every other tile
            if ((x + z) % 2 == 0)
            {
                float trunkHeight = 1.5f;
                float foliageHeight = 1.5f;

                // Trunk
                sRenderItem trunkItem;
                trunkItem.pGeometry = pMeshGeo;
                trunkItem.pMaterial = &trunkMaterial;   // Assign trunk material
                trunkItem.objCBIndex = static_cast<UINT>(m_renderItems.size());

                const auto& cylSubmesh = pMeshGeo->drawArguments.at("cylinder");
                trunkItem.indexCount = cylSubmesh.indexCount;
                trunkItem.startIndexLocation = cylSubmesh.startIndexLocation;
                trunkItem.baseVertexLocation = cylSubmesh.startVertexLocation;

                XMMATRIX trunkScale = XMMatrixScaling(0.5f, trunkHeight * 0.5f, 0.5f);
                XMMATRIX trunkTranslate = XMMatrixTranslation(x * spacing, trunkHeight * 0.25f, z * spacing);
                XMStoreFloat4x4(&trunkItem.worldMatrix, trunkScale * trunkTranslate);
                m_renderItems.emplace_back(std::move(trunkItem));

                // Foliage
                sRenderItem foliageItem;
                foliageItem.pGeometry = pMeshGeo;
                foliageItem.pMaterial = &foliageMaterial;   // Assign foliage material
                foliageItem.objCBIndex = static_cast<UINT>(m_renderItems.size());

                const auto& sphereSubmesh = pMeshGeo->drawArguments.at("sphere");
                foliageItem.indexCount = sphereSubmesh.indexCount;
                foliageItem.startIndexLocation = sphereSubmesh.startIndexLocation;
                foliageItem.baseVertexLocation = sphereSubmesh.startVertexLocation;

                XMMATRIX foliageScale = XMMatrixScaling(foliageHeight, foliageHeight, foliageHeight);
                XMMATRIX foliageTranslate = XMMatrixTranslation(x * spacing, trunkHeight + foliageHeight * 0.5f, z * spacing);
                XMStoreFloat4x4(&foliageItem.worldMatrix, foliageScale * foliageTranslate);
                m_renderItems.emplace_back(std::move(foliageItem));
            }
        }
    }
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

    m_pDirectX12->Update(view, &m_renderItems, camPos);
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


