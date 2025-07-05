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
	m_pDirectX12->Initialize(m_pWindow, m_pTimer, c_numberOfRenderItems, 1000);


    std::cout << "init to win it" << std::endl;

	InitializeRenderItems();
    InitializeLights();
    InitializeLightVisualizations();
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
        InitializeLights();
        InitializeLightVisualizations();
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
    m_pDirectX12->InitializeMesh(meshGenerator.CreateCube(), std::string("cube"), XMFLOAT4(0.4f, 0.8f, 0.4f, 1.f));                                     // green ground
    m_pDirectX12->InitializeMesh(meshGenerator.CreateSphere(1.0f, 20, 20), std::string("sphere"), XMFLOAT4(0.1f, 0.6f, 0.1f, 1.f));                     // foliage
    m_pDirectX12->InitializeMesh(meshGenerator.CreateCylinder(0.3, 0.3, 1.2f, 20, 20), std::string("cylinder"), XMFLOAT4(0.55f, 0.27f, 0.07f, 1.f));    // trunk

    sMeshGeometry* pMeshGeo = m_pDirectX12->InitializeGeometryBuffer();

    // Materials (same as before)
    static sMaterial groundMaterial;
    groundMaterial.albedo = XMFLOAT3(0.4f, 0.8f, 0.4f);
    groundMaterial.specularExponent = 16.0f;

    static sMaterial trunkMaterial;
    trunkMaterial.albedo = XMFLOAT3(0.55f, 0.27f, 0.07f);
    trunkMaterial.specularExponent = 32.0f;

    static sMaterial foliageMaterial;
    foliageMaterial.albedo = XMFLOAT3(0.1f, 0.6f, 0.1f);
    foliageMaterial.specularExponent = 8.0f;

    const int groundRows = 40;
    const int groundCols = 40;
    const float spacing = 5.0f;

    m_renderItems.clear();
    m_renderItems.reserve(groundRows * groundCols + 5000);

    // Generate ground + trees (same as your code)
    for (int z = 0; z < groundRows; ++z)
    {
        for (int x = 0; x < groundCols; ++x)
        {
            // Ground cube
            sRenderItem cubeItem;
            cubeItem.pGeometry = pMeshGeo;
            cubeItem.pMaterial = &groundMaterial;
            cubeItem.objCBIndex = static_cast<UINT>(m_renderItems.size());

            const auto& submesh = pMeshGeo->drawArguments.at("cube");
            cubeItem.indexCount = submesh.indexCount;
            cubeItem.startIndexLocation = submesh.startIndexLocation;
            cubeItem.baseVertexLocation = submesh.startVertexLocation;

            XMMATRIX scale = XMMatrixScaling(5.f, 0.5f, 5.f);
            XMMATRIX translate = XMMatrixTranslation(x * spacing, -0.25f, z * spacing);
            XMStoreFloat4x4(&cubeItem.worldMatrix, scale * translate);
            m_renderItems.emplace_back(std::move(cubeItem));

            // Trees (trunk + foliage)
            if ((x + z) % 2 == 0)
            {
                // Trunk
                sRenderItem trunkItem;
                trunkItem.pGeometry = pMeshGeo;
                trunkItem.pMaterial = &trunkMaterial;
                trunkItem.objCBIndex = static_cast<UINT>(m_renderItems.size());

                const auto& cylSubmesh = pMeshGeo->drawArguments.at("cylinder");
                trunkItem.indexCount = cylSubmesh.indexCount;
                trunkItem.startIndexLocation = cylSubmesh.startIndexLocation;
                trunkItem.baseVertexLocation = cylSubmesh.startVertexLocation;

                float trunkHeight = 1.5f;
                XMMATRIX trunkScale = XMMatrixScaling(0.5f, trunkHeight * 0.5f, 0.5f);
                XMMATRIX trunkTranslate = XMMatrixTranslation(x * spacing, trunkHeight * 0.25, z * spacing);
                XMStoreFloat4x4(&trunkItem.worldMatrix, trunkScale * trunkTranslate);
                m_renderItems.emplace_back(std::move(trunkItem));

                // Foliage
                sRenderItem foliageItem;
                foliageItem.pGeometry = pMeshGeo;
                foliageItem.pMaterial = &foliageMaterial;
                foliageItem.objCBIndex = static_cast<UINT>(m_renderItems.size());

                const auto& sphereSubmesh = pMeshGeo->drawArguments.at("sphere");
                foliageItem.indexCount = sphereSubmesh.indexCount;
                foliageItem.startIndexLocation = sphereSubmesh.startIndexLocation;
                foliageItem.baseVertexLocation = sphereSubmesh.startVertexLocation;

                float foliageHeight = 1.5f;
                XMMATRIX foliageScale = XMMatrixScaling(foliageHeight, foliageHeight, foliageHeight);
                XMMATRIX foliageTranslate = XMMatrixTranslation(x * spacing, trunkHeight + foliageHeight * 0.5f, z * spacing);
                XMStoreFloat4x4(&foliageItem.worldMatrix, foliageScale * foliageTranslate);
                m_renderItems.emplace_back(std::move(foliageItem));
            }
        }
    }

    // Mark where light visualizations start (initially at the end)
    m_lightVisStartIndex = m_renderItems.size();
}

// --------------------------------------------------------------------------------------------------------------------------
void cSystem::InitializeLights()
{
    // --- Spotlights in circle around center (100,0,100) ---
    const int totalSpotlights = 50;
    const float spotlightRadius = 100.0f; // radius around center
    const float spotlightHeight = 30.0f;

    // --- Point lights in grid ---
    const int totalPointLights = 400;  // 20x20 grid
    const int gridSize = 20;
    const float pointLightSpacing = 8.0f;
    const float pointLightHeight = 10.0f;

    // Offsets for point lights
    const float pointLightOffsetX = 20.0f;
    const float pointLightOffsetZ = 20.0f;

    // Offsets for spotlights (new separate variables)
    const float spotlightOffsetX = 100.0f;
    const float spotlightOffsetZ = 100.0f;

    m_lights.clear();
    m_lights.resize(totalSpotlights + totalPointLights);

    auto HSVtoRGB = [](float h, float s, float v) -> XMFLOAT3
        {
            float c = v * s;
            float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
            float m = v - c;
            float r, g, b;

            if (h < 60) { r = c; g = x; b = 0; }
            else if (h < 120) { r = x; g = c; b = 0; }
            else if (h < 180) { r = 0; g = c; b = x; }
            else if (h < 240) { r = 0; g = x; b = c; }
            else if (h < 300) { r = x; g = 0; b = c; }
            else { r = c; g = 0; b = x; }

            return XMFLOAT3(r + m, g + m, b + m);
        };

    float time = static_cast<float>(m_pTimer->GetTotalTime());

    // Spotlights in circle around (100, 0, 100)
    for (int i = 0; i < totalSpotlights; ++i)
    {
        float angle = ((float)i / totalSpotlights) * XM_2PI;

        float x = cosf(angle) * spotlightRadius + spotlightOffsetX;
        float z = sinf(angle) * spotlightRadius + spotlightOffsetZ;

        float y = spotlightHeight + 10.0f * sinf(time * 3.0f + i);

        XMFLOAT3 pos(x, y, z);
        XMFLOAT3 target(spotlightOffsetX, 0.0f, spotlightOffsetZ);

        XMVECTOR posVec = XMLoadFloat3(&pos);
        XMVECTOR targetVec = XMLoadFloat3(&target);
        XMVECTOR dirVec = XMVector3Normalize(XMVectorSubtract(targetVec, posVec));

        XMFLOAT3 direction;
        XMStoreFloat3(&direction, dirVec);

        float hue = fmod(time * 120.0f + i * (360.0f / totalSpotlights), 360.0f);
        float saturation = 1.0f;
        float brightness = 1.0f;

        XMFLOAT3 color = HSVtoRGB(hue, saturation, brightness);

        // Multiply color strength by 3x to make lights much stronger
        color.x *= 3.0f;
        color.y *= 3.0f;
        color.z *= 3.0f;

        m_lights[i].position = pos;
        m_lights[i].direction = direction;
        m_lights[i].strength = color;

        // Increase range for stronger effect
        m_lights[i].falloffStart = 15.0f;
        m_lights[i].falloffEnd = 150.0f;

        // Make spotlight cone very sharp and bright
        m_lights[i].spotPower = 256.0f;

        m_lights[i].type = 2; // spotlight
    }

    // Point lights grid starts after spotlights
    for (int i = 0; i < totalPointLights; ++i)
    {
        int index = i + totalSpotlights;
        int row = i / gridSize;
        int col = i % gridSize;

        float x = col * pointLightSpacing + pointLightOffsetX;
        float y = pointLightHeight;
        float z = row * pointLightSpacing + pointLightOffsetZ;

        float hue = fmod(360.0f * ((float)col / gridSize) + time * 30.0f, 360.0f);
        float saturation = 0.8f;
        float brightness = 0.4f + 0.3f * sinf(time * 4.0f + i);

        XMFLOAT3 color = HSVtoRGB(hue, saturation, brightness);

        m_lights[index].position = XMFLOAT3(x, y, z);
        m_lights[index].direction = XMFLOAT3(0, 0, 0);
        m_lights[index].strength = color;
        m_lights[index].falloffStart = 1.0f;
        m_lights[index].falloffEnd = 30.0f;
        m_lights[index].spotPower = 1.0f;
        m_lights[index].type = 1; // point light
    }
}

// ----------------------------------------------------------------
void cSystem::InitializeLightVisualizations()
{
    // Remove only old light visualization render items, keep scene intact
    m_renderItems.erase(m_renderItems.begin() + m_lightVisStartIndex, m_renderItems.end());

    sMeshGeometry* pMeshGeo = m_pDirectX12->GetGeometry();

    const float lightSphereScale = 0.3f;
    const float spotlightDirLength = 5.0f;
    const float spotlightDirRadius = 0.05f;

    static sMaterial spotlightMat;
    static bool spotlightMatInitialized = false;
    if (!spotlightMatInitialized)
    {
        spotlightMat.albedo = XMFLOAT3(0.0f, 0.5f, 1.0f);
        spotlightMat.specularExponent = 16.0f;
        spotlightMatInitialized = true;
    }

    static sMaterial pointLightMat;
    static bool pointLightMatInitialized = false;
    if (!pointLightMatInitialized)
    {
        pointLightMat.albedo = XMFLOAT3(1.0f, 0.8f, 0.3f);
        pointLightMat.specularExponent = 8.0f;
        pointLightMatInitialized = true;
    }

    const auto& sphereSubmesh = pMeshGeo->drawArguments.at("sphere");
    const auto& cylinderSubmesh = pMeshGeo->drawArguments.at("cylinder");

    // Start adding light visualization render items at the saved index
    m_lightVisStartIndex = m_renderItems.size();

    for (size_t i = 0; i < m_lights.size(); ++i)
    {
        if (m_lights[i].type == 2) // spotlight
        {
            // Sphere at spotlight position
            sRenderItem spotlightVis;
            spotlightVis.pGeometry = pMeshGeo;
            spotlightVis.pMaterial = &spotlightMat;
            spotlightVis.objCBIndex = static_cast<UINT>(m_renderItems.size());

            spotlightVis.indexCount = sphereSubmesh.indexCount;
            spotlightVis.startIndexLocation = sphereSubmesh.startIndexLocation;
            spotlightVis.baseVertexLocation = sphereSubmesh.startVertexLocation;

            XMMATRIX scale = XMMatrixScaling(lightSphereScale, lightSphereScale, lightSphereScale);
            XMMATRIX translate = XMMatrixTranslation(m_lights[i].position.x, m_lights[i].position.y, m_lights[i].position.z);
            XMStoreFloat4x4(&spotlightVis.worldMatrix, scale * translate);

            m_renderItems.emplace_back(std::move(spotlightVis));

            // Cylinder for spotlight direction
            sRenderItem dirVis;
            dirVis.pGeometry = pMeshGeo;
            dirVis.pMaterial = &spotlightMat;
            dirVis.objCBIndex = static_cast<UINT>(m_renderItems.size());

            dirVis.indexCount = cylinderSubmesh.indexCount;
            dirVis.startIndexLocation = cylinderSubmesh.startIndexLocation;
            dirVis.baseVertexLocation = cylinderSubmesh.startVertexLocation;

            XMVECTOR spotDir = XMLoadFloat3(&m_lights[i].direction);
            spotDir = XMVector3Normalize(spotDir);

            XMVECTOR defaultDir = XMVectorSet(0, 1, 0, 0);
            XMVECTOR axis = XMVector3Cross(defaultDir, spotDir);
            float angle = acosf(XMVectorGetX(XMVector3Dot(defaultDir, spotDir)));

            XMMATRIX rotation = XMMatrixIdentity();
            if (XMVectorGetX(XMVector3Length(axis)) > 0.0001f)
            {
                axis = XMVector3Normalize(axis);
                rotation = XMMatrixRotationAxis(axis, angle);
            }
            else if (angle > XM_PI / 2)
            {
                rotation = XMMatrixRotationAxis(XMVectorSet(1, 0, 0, 0), XM_PI);
            }

            XMVECTOR cylPos = XMLoadFloat3(&m_lights[i].position) + spotDir * (spotlightDirLength * 0.5f);

            XMMATRIX scaleCyl = XMMatrixScaling(spotlightDirRadius, spotlightDirLength, spotlightDirRadius);
            XMMATRIX translateCyl = XMMatrixTranslationFromVector(cylPos);
            XMStoreFloat4x4(&dirVis.worldMatrix, scaleCyl * rotation * translateCyl);

            m_renderItems.emplace_back(std::move(dirVis));
        }
    }

    // Point lights
    for (size_t i = 0; i < m_lights.size(); ++i)
    {
        if (m_lights[i].type == 1) // point light
        {
            sRenderItem pointVis;
            pointVis.pGeometry = pMeshGeo;
            pointVis.pMaterial = &pointLightMat;
            pointVis.objCBIndex = static_cast<UINT>(m_renderItems.size());

            pointVis.indexCount = sphereSubmesh.indexCount;
            pointVis.startIndexLocation = sphereSubmesh.startIndexLocation;
            pointVis.baseVertexLocation = sphereSubmesh.startVertexLocation;

            const XMFLOAT3& pos = m_lights[i].position;
            XMMATRIX scale = XMMatrixScaling(lightSphereScale * 0.5f, lightSphereScale * 0.5f, lightSphereScale * 0.5f);
            XMMATRIX translate = XMMatrixTranslation(pos.x, pos.y, pos.z);
            XMStoreFloat4x4(&pointVis.worldMatrix, scale * translate);

            m_renderItems.emplace_back(std::move(pointVis));
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


