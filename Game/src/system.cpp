#include "system.h"

#include <algorithm>
#include <iostream>
#include <DirectXColors.h>
#include <wrl.h>
#include <d3d12.h>

#include "core/window.h"
#include "graphics/directx12.h"
#include "graphics/directx12Util.h"
#include "core/timer.h"
#include "core/input.h"
#include "graphics/vertex.h"
#include "Graphics/meshGeometry.h"

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


	m_radius = 10.0f;    // Reasonable camera distance
	m_theta = 1.5f;      // Some rotation around Y
	m_phi = 1.0f;
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

    const int totalItems = 1000;
    const float spacing = 4.0f;     
	const int itemsPerRow = 20;
    const int itemsPerLayer = 20;

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

        XMMATRIX scale = XMMatrixScaling(2.f, 2.f, 2.f);
        XMMATRIX translation = XMMatrixTranslation(xPos, yPos, zPos);
        XMMATRIX world = scale * translation;

        switch (typeIndex)
        {
        case 0: // cube
        {
            const sSubmeshGeometry& submesh = pMeshGeo->drawArguments.at("cube");
            renderItem.indexCount = submesh.indexCount;
            renderItem.startIndexLocation = submesh.startIndexLocation;
            renderItem.baseVertexLocation = submesh.startVertexLocation;
            break;
        }
        case 1: // sphere
        {
            const sSubmeshGeometry& submesh = pMeshGeo->drawArguments.at("sphere");
            renderItem.indexCount = submesh.indexCount;
            renderItem.startIndexLocation = submesh.startIndexLocation;
            renderItem.baseVertexLocation = submesh.startVertexLocation;
            break;
        }
        case 2: // cylinder
        {
            const sSubmeshGeometry& submesh = pMeshGeo->drawArguments.at("cylinder");
            renderItem.indexCount = submesh.indexCount;
            renderItem.startIndexLocation = submesh.startIndexLocation;
            renderItem.baseVertexLocation = submesh.startVertexLocation;
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

	// Convert spherical to Cartesian coordinates
	float x = m_radius * sinf(m_phi) * cosf(m_theta);
	float y = m_radius * sinf(m_phi) * sinf(m_theta);
	float z = m_radius * cosf(m_phi);

	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR at = XMVectorZero(); 
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); 

	// Create and store the view matrix
	XMMATRIX view = XMMatrixLookAtLH(pos, at, up);
	XMStoreFloat4x4(&m_view, view);

	// Update DirectX12 engine
	m_pDirectX12->Update(view, &m_renderItems);
}


// --------------------------------------------------------------------------------------------------------------------------

void cSystem::HandleInput()
{
	if (cInput::IsMouseButtonDown(MK_LBUTTON))
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(cInput::GetMouseX() - m_lastMouseX));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(cInput::GetMouseY() - m_lastMouseY));

		m_theta += dx;
		m_phi += dy;

		m_phi = std::clamp(m_phi, 0.1f, XM_PI - 0.1f);
	}
	else if (cInput::IsMouseButtonDown(MK_RBUTTON))
	{
		float dy = 0.25f * static_cast<float>(cInput::GetMouseY() - m_lastMouseY);

		m_radius += 0.01f * dy;

		// Final clamp: enforce positive, reasonable zoom
		m_radius = std::clamp(m_radius, 3.0f, 15.0f);
	}

	m_lastMouseX = cInput::GetMouseX();
	m_lastMouseY = cInput::GetMouseY();
}



