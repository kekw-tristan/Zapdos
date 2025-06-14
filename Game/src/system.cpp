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

    // Initialize meshes with distinct keys
    m_pDirectX12->InitializeMesh(meshGenerator.CreateCube(), std::string("cube"), XMFLOAT4(0.f, 0.f, 1.f, 1.f));
    m_pDirectX12->InitializeMesh(meshGenerator.CreateSphere(1.0f, 20, 20), std::string("sphere"), XMFLOAT4(0.f, 1.f, 1.f, 1.f));
    m_pDirectX12->InitializeMesh(meshGenerator.CreateCylinder(0.3, 0.5, 5.f, 20, 20), std::string("cylinder"), XMFLOAT4(1.f, 0.f, 1.f, 1.f));

    sMeshGeometry* pMeshGeo = m_pDirectX12->InitializeGeometryBuffer();

    const int   totalItems      = 1000;
    const float spacing         = 4.0f;     
	const int   itemsPerRow     = 20;
    const int   itemsPerLayer   = 20;

    for (int i = 0; i < totalItems; ++i)
    {
        sRenderItem renderItem;
        renderItem.pGeometry = pMeshGeo;
        renderItem.objCBIndex = static_cast<UINT>(i);

        // Cycle through cube, sphere, cylinder
        int typeIndex = i % 3;

        // Calculate 3D grid position:
        int xIndex = i % itemsPerRow;
        int yIndex = (i / itemsPerRow) % itemsPerLayer;
        int zIndex = i / (itemsPerRow * itemsPerLayer);

        float xPos = (xIndex - itemsPerRow / 2) * spacing;
        float yPos = (yIndex - itemsPerLayer / 2) * spacing;
        float zPos = (zIndex - (totalItems / (itemsPerRow * itemsPerLayer)) / 2) * spacing;

        XMMATRIX scale          = XMMatrixScaling(2.f, 2.f, 2.f);
        XMMATRIX translation    = XMMatrixTranslation(xPos, yPos, zPos);
        XMMATRIX world          = scale * translation;

        switch (typeIndex)
        {
            case 0: // cube
            {
                const sSubmeshGeometry& submesh = pMeshGeo->drawArguments.at("cube");
                renderItem.indexCount           = submesh.indexCount;
                renderItem.startIndexLocation   = submesh.startIndexLocation;
                renderItem.baseVertexLocation   = submesh.startVertexLocation;
                break;
            }
            case 1: // sphere
            {
                const sSubmeshGeometry& submesh = pMeshGeo->drawArguments.at("sphere");
                renderItem.indexCount           = submesh.indexCount;
                renderItem.startIndexLocation   = submesh.startIndexLocation;
                renderItem.baseVertexLocation   = submesh.startVertexLocation;
                break;
            }
            case 2: // cylinder
            {
                const sSubmeshGeometry& submesh     = pMeshGeo->drawArguments.at("cylinder");
                renderItem.indexCount               = submesh.indexCount;
                renderItem.startIndexLocation       = submesh.startIndexLocation;
                renderItem.baseVertexLocation       = submesh.startVertexLocation;
                break;
            }
            default:
                break;
        }

        XMStoreFloat4x4(&renderItem.worldMatrix, world);
        m_renderItems[i] = renderItem;
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

    m_pDirectX12->Update(view, &m_renderItems);
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


