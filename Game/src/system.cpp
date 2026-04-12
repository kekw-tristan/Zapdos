#include "system.h"

#include <algorithm>
#include <iostream>
#include <iterator>
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

    // Falls die Funktion mehrfach aufgerufen wird, musst du vorher auch
    // deine alten RenderItems / Geometrie-Sammler zurücksetzen.
    m_materials.clear();
    m_textures.clear();
    m_pScene->GetRenderItems().clear();

    struct sLoadedModel
    {
        std::vector<cMeshGenerator::sMeshData> meshes;
        std::vector<sMaterial> materials;
        std::vector<XMMATRIX> worldMatrices;
        std::vector<cTexture> textures;
    };

    auto ApplyMaterialDefaults = [](std::vector<sMaterial>& materials)
        {
            for (auto& mat : materials)
            {
                mat.alpha = (mat.alpha == 0.f) ? 1.f : mat.alpha;
                mat.roughness = (mat.roughness == 0.f) ? 0.5f : mat.roughness;
                mat.ao = (mat.ao == 0.f) ? 1.f : mat.ao;
            }
        };

    auto OffsetMaterialTextureIndices = [](std::vector<sMaterial>& materials, int textureBase)
        {
            for (auto& mat : materials)
            {
                if (mat.baseColorIndex >= 0)
                    mat.baseColorIndex += textureBase;

                // Falls du später normal/metallicRoughness/emissive Indizes im Material hast,
                // hier ebenfalls offsetten.
            }
        };

    auto OffsetMeshMaterialIds = [](std::vector<cMeshGenerator::sMeshData>& meshes, UINT materialBase)
        {
            for (auto& mesh : meshes)
            {
                if (mesh.materialId != UINT_MAX)
                    mesh.materialId += materialBase;
            }
        };

    auto LoadModel = [&](const std::string& path, sLoadedModel& outModel)
        {
            meshGenerator.LoadModelFromGLTF(
                const_cast<std::string&>(path),
                outModel.meshes,
                outModel.materials,
                outModel.worldMatrices,
                outModel.textures,
                m_pDirectX12->GetDevice());

            ApplyMaterialDefaults(outModel.materials);

            assert(outModel.meshes.size() == outModel.worldMatrices.size());
        };

    // ------------------------------------------------------------
    // 1) Modelle getrennt laden
    // ------------------------------------------------------------
    sLoadedModel sceneModel;
    sLoadedModel sphereModel;

    LoadModel("..\\Assets\\Objects\\scene.gltf", sceneModel);
    LoadModel("..\\Assets\\Objects\\sphere.gltf", sphereModel);

    // ------------------------------------------------------------
    // 2) Materialien/Texturen in globale Arrays übernehmen
    //    und IDs/Indizes passend offsetten
    // ------------------------------------------------------------

    // scene.gltf
    {
        const UINT materialBase = static_cast<UINT>(m_materials.size());
        const int  textureBase = static_cast<int>(m_textures.size());

        OffsetMaterialTextureIndices(sceneModel.materials, textureBase);
        OffsetMeshMaterialIds(sceneModel.meshes, materialBase);

        m_materials.insert(m_materials.end(), sceneModel.materials.begin(), sceneModel.materials.end());
        m_textures.insert(
            m_textures.end(),
            std::make_move_iterator(sceneModel.textures.begin()),
            std::make_move_iterator(sceneModel.textures.end()));
    }

    // sphere.gltf
    {
        const UINT materialBase = static_cast<UINT>(m_materials.size());
        const int  textureBase = static_cast<int>(m_textures.size());

        OffsetMaterialTextureIndices(sphereModel.materials, textureBase);
        OffsetMeshMaterialIds(sphereModel.meshes, materialBase);

        m_materials.insert(m_materials.end(), sphereModel.materials.begin(), sphereModel.materials.end());
        m_textures.insert(
            m_textures.end(),
            std::make_move_iterator(sphereModel.textures.begin()),
            std::make_move_iterator(sphereModel.textures.end()));
    }

    // ------------------------------------------------------------
    // 3) Geometrie einmal gesammelt aufbauen
    // ------------------------------------------------------------
    const size_t sceneSubmeshBase = 0;
    for ( auto& mesh : sceneModel.meshes)
    {
        m_pDirectX12->InitializeMesh(mesh);
    }

    const size_t sphereSubmeshBase = sceneModel.meshes.size();
    for ( auto& mesh : sphereModel.meshes)
    {
        m_pDirectX12->InitializeMesh(mesh);
    }

    sMeshGeometry* pMeshGeo = m_pDirectX12->InitializeGeometryBuffer();
    m_pDirectX12->UploadTexturesToGPU(m_textures);

    static sMaterial defaultMaterial;
    defaultMaterial.albedo = XMFLOAT3(1.f, 1.f, 1.f);
    defaultMaterial.alpha = 1.f;
    defaultMaterial.metallic = 0.f;
    defaultMaterial.roughness = 0.5f;
    defaultMaterial.ao = 1.f;
    defaultMaterial.emissive = XMFLOAT3(0.f, 0.f, 0.f);

    std::cout << "scene meshes:  " << sceneModel.meshes.size() << std::endl;
    std::cout << "sphere meshes: " << sphereModel.meshes.size() << std::endl;
    std::cout << "drawArguments: " << pMeshGeo->drawArguments.size() << std::endl;
    std::cout << "materials:     " << m_materials.size() << std::endl;
    std::cout << "textures:      " << m_textures.size() << std::endl;

    assert(pMeshGeo->drawArguments.size() == sceneModel.meshes.size() + sphereModel.meshes.size());

    auto AddRenderItems =
        [&](const std::vector<cMeshGenerator::sMeshData>& meshes,
            const std::vector<XMMATRIX>& worldMatrices,
            size_t submeshBase,
            const XMMATRIX& instanceOffset,
            UINT& objCBIndex)
        {
            for (size_t i = 0; i < meshes.size(); ++i)
            {
                sRenderItem ri{};

                ri.pGeometry = pMeshGeo;
                ri.objCBIndex = objCBIndex++;

                const UINT matIndex = meshes[i].materialId;
                if (matIndex < m_materials.size())
                    ri.pMaterial = &m_materials[matIndex];
                else
                    ri.pMaterial = &defaultMaterial;

                const auto& submesh = pMeshGeo->drawArguments[submeshBase + i];
                ri.indexCount = submesh.indexCount;
                ri.startIndexLocation = submesh.startIndexLocation;
                ri.baseVertexLocation = submesh.startVertexLocation;

                XMMATRIX finalWorld = worldMatrices[i] * instanceOffset;
                XMStoreFloat4x4(&ri.worldMatrix, finalWorld);

                ri.numberOfFramesDirty = c_NumberOfFrameResources;

                m_pScene->GetRenderItems().emplace_back(std::move(ri));
            }
        };

    // ------------------------------------------------------------
    // 4) RenderItems erzeugen
    // ------------------------------------------------------------
    UINT objCBIndex = 0;

    // scene.gltf: original
    AddRenderItems(
        sceneModel.meshes,
        sceneModel.worldMatrices,
        sceneSubmeshBase,
        XMMatrixIdentity(),
        objCBIndex);

    // scene.gltf: zweites Mal bei +10 auf X
    AddRenderItems(
        sceneModel.meshes,
        sceneModel.worldMatrices,
        sceneSubmeshBase,
        XMMatrixTranslation(100.0f, 0.0f, 0.0f),
        objCBIndex);

    // sphere.gltf: einmal bei +20 auf X
    AddRenderItems(
        sphereModel.meshes,
        sphereModel.worldMatrices,
        sphereSubmeshBase,
        XMMatrixTranslation(50.0f, 0.0f, 0.0f),
        objCBIndex);

    std::cout << "scene.gltf zweimal + sphere.gltf einmal erfolgreich eingebaut.\n";
}

// --------------------------------------------------------------------------------------------------------------------------

void cSystem::InitializeLights()
{
    auto& lights = m_pScene->GetLight();

    sLightConstants directionalLight{};
    directionalLight.strength = XMFLOAT3(1, 1, 1);
    directionalLight.direction = XMFLOAT3(1, -1, 1);
    directionalLight.type = 0;

    lights.push_back(directionalLight);

    const int pointLightCount = 0;
    const float radius = 5.f;
    const float yHeight = 2.f;

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