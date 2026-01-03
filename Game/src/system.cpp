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

    m_pScene  = std::make_unique<cScene>();
    m_pCamera = std::make_unique<cCamera>();
    m_pCamera->SetProjection(XM_PI / 4.f, 1280.f / 720.f, 0.1f, 1000.f);

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

        float deltaTime = m_pTimer->GetDeltaTime();
        Update(deltaTime);

        m_pDirectX12->Draw();
    }
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Finalize()
{
    std::cout << "Finalize\n";

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
    std::vector<sMaterial> materials;

    std::string path = "..\\Assets\\Objects\\scene.gltf";
    meshGenerator.LoadModelFromGLTF(path, meshes, materials, worldMatrices);

    // Initialize GPU meshes
    for (int i = 0; i < meshes.size(); ++i)
    {
        std::string name = "scene_" + std::to_string(i);
        m_pDirectX12->InitializeMesh(meshes[i], name, XMFLOAT4(0.4f, 0.8f, 0.4f, 1.f));
    }

    sMeshGeometry* pMeshGeo = m_pDirectX12->InitializeGeometryBuffer();

    // Create render items
    for (int i = 0; i < meshes.size(); ++i)
    {
        sRenderItem ri;
        std::string name = "scene_" + std::to_string(i);

        ri.pGeometry = pMeshGeo;
        ri.objCBIndex = static_cast<UINT>(i);

        int matIndex = meshes[i].materialIndex;
        if (matIndex >= 0 && matIndex < materials.size())
        {
            ri.pMaterial = &materials[matIndex];
        }
        else
        {
            static sMaterial defaultMaterial;
            ri.pMaterial = &defaultMaterial;
        }

        const auto& submesh = pMeshGeo->drawArguments.at(name);
        ri.indexCount = submesh.indexCount;
        ri.startIndexLocation = submesh.startIndexLocation;
        ri.baseVertexLocation = submesh.startVertexLocation;

        XMStoreFloat4x4(&ri.worldMatrix, worldMatrices[i]);

        m_pScene->GetRenderItems().emplace_back(std::move(ri));
    }

    std::cout << "Meshes & materials initialized (glTF PBR)\n";
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

    const int pointLightCount = 10;
    const float radius = 30.f;
    const float yHeight = 20.f;

    for (int i = 0; i < pointLightCount; ++i)
    {
        sLightConstants light{};
        float angle = XM_2PI * i / pointLightCount;

        light.position = XMFLOAT3(radius * cosf(angle), yHeight, radius * sinf(angle));
        light.strength = XMFLOAT3(1, 1, 1);
        light.falloffStart = 10.f;
        light.falloffEnd = 50.f;
        light.type = 1;

        lights.push_back(light);
    }
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::Update(float deltaTime)
{
    HandleInput(deltaTime);

    XMMATRIX view = m_pCamera->GetViewMatrix();
    XMFLOAT3 camPos = m_pCamera->GetPosition();

    m_pDirectX12->Update(view, camPos, &m_pScene->GetRenderItems(), &m_pScene->GetLight());
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::HandleInput(float deltaTime)
{
    float speed = 30.f * deltaTime;
    float rotSpeed = (150.0f * XM_PI / 180.0f) * deltaTime;

    XMFLOAT3 moveDelta{ 0.f,0.f,0.f };
    float pitchDelta = 0.f;
    float yawDelta = 0.f;

    // WASD movement
    if (cInput::IsKeyDown('W')) moveDelta.z += 1.f;
    if (cInput::IsKeyDown('S')) moveDelta.z -= 1.f;
    if (cInput::IsKeyDown('A')) moveDelta.x -= 1.f;
    if (cInput::IsKeyDown('D')) moveDelta.x += 1.f;

    // Arrow rotation
    if (cInput::IsKeyDown(VK_UP))    pitchDelta -= 1.f;
    if (cInput::IsKeyDown(VK_DOWN))  pitchDelta += 1.f;
    if (cInput::IsKeyDown(VK_LEFT))  yawDelta -= 1.f;
    if (cInput::IsKeyDown(VK_RIGHT)) yawDelta += 1.f;

    m_pCamera->Move(XMFLOAT3(moveDelta.x * speed, moveDelta.y * speed, moveDelta.z * speed));
    m_pCamera->Rotate(pitchDelta * rotSpeed, yawDelta * rotSpeed);
}