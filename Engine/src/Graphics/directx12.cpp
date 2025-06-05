#include "directx12.h"

#include <array>
#include <dxgidebug.h>
#include <iostream>
#include <string>
#include <comdef.h>
#include <windows.h>
#include <d3dx12.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>
#include <shlobj.h>
#include <filesystem>

#include "core/window.h"
#include "core/timer.h"
#include "Core/input.h"

#include "directx12Util.h"
#include "frameResource.h"
#include "swapChainManager.h"
#include "deviceManager.h"
#include "pipelineManager.h"
#include "bufferManager.h"
#include "renderItem.h"
#include "uploadBuffer.h"
#include "vertex.h"
#include "meshGeometry.h"
#include "meshGenerator.h"

constexpr float c_pi = 3.1415927f;

// possible things to add:
// sissor rectangles(pixels outside the rectangle area are culled [useful for gui optimization])

// --------------------------------------------------------------------------------------------------------------------------

static std::wstring GetLatestWinPixGpuCapturerPath_Cpp17()
{
    LPWSTR programFilesPath = nullptr;
    SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

    std::filesystem::path pixInstallationPath = programFilesPath;
    pixInstallationPath /= "Microsoft PIX";

    std::wstring newestVersionFound;

    for (auto const& directory_entry : std::filesystem::directory_iterator(pixInstallationPath))
    {
        if (directory_entry.is_directory())
        {
            if (newestVersionFound.empty() || newestVersionFound < directory_entry.path().filename().c_str())
            {
                newestVersionFound = directory_entry.path().filename().c_str();
            }
        }
    }

    if (newestVersionFound.empty())
    {
        // TODO: Error, no PIX installation found
        std::cout << "no pix installation found" << std::endl;
    }

    return pixInstallationPath / newestVersionFound / L"WinPixGpuCapturer.dll";
}

// --------------------------------------------------------------------------------------------------------------------------
// initializes all the directx12 components

void cDirectX12::Initialize(cWindow* _pWindow, cTimer* _pTimer)
{
    if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
    {
        LoadLibrary(GetLatestWinPixGpuCapturerPath_Cpp17().c_str());
    }
    // activate debug layer
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        std::cout << "D3D12 Debug Layer enabled." << std::endl;
    }
    else
    {
        std::cerr << "D3D12 Debug Layer not available." << std::endl;
    }
   
    m_pWindow = _pWindow;
    m_pTimer = _pTimer;

    m_pDeviceManager = new cDeviceManager();
    m_pDeviceManager->Initialize();

    m_pSwapChainManager = new cSwapChainManager(m_pDeviceManager, m_pWindow);
    m_pSwapChainManager->Initialize();

    m_pBufferManager = new cBufferManager(m_pDeviceManager, m_pSwapChainManager);
    m_pBufferManager->Initialize();

    m_pPipelineManager = new cPipelineManager(m_pDeviceManager);
    m_pPipelineManager->Initialize(); 
    
    InitializeVertices();
    InitializeFrameResources();

    m_pDeviceManager->GetCommandList()->Close();
    ID3D12CommandList* cmdLists[] = { m_pDeviceManager->GetCommandList() };
    m_pDeviceManager->GetCommandQueue()->ExecuteCommandLists(_countof(cmdLists), cmdLists);
    m_pDeviceManager->FlushCommandQueue();
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::Finalize()
{
    WaitForGPU();

    for (auto* renderItem : m_renderItems)
        delete renderItem;

    for (auto* frameResource : m_frameResources)
    {
        delete frameResource;
    }

    for (auto& pair : m_geometries)
    {
        delete pair.second;
    }
    
    delete m_pBoxGeometry;

    delete m_pBufferManager;
    delete m_pSwapChainManager;
    delete m_pDeviceManager;
    delete m_pPipelineManager;
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::InitializeVertices()
{

    cMeshGenerator meshGenerator;

    // Create Cylinder
    cMeshGenerator::sMeshData cylinder = meshGenerator.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

    // Setup submesh info for cylinder
    UINT cylinderVertexOffset = 0;
    UINT cylinderIndexOffset = 0;

    sSubmeshGeometry cylinderSubmesh;
    cylinderSubmesh.indexCount = static_cast<UINT>(cylinder.indices32.size());
    cylinderSubmesh.startIndexLocation = cylinderIndexOffset;
    cylinderSubmesh.startVertexLocation = cylinderVertexOffset;

    // Convert cylinder vertices
    std::vector<sVertex> vertecies;
    for (const auto& v : cylinder.vertecies)
    {
        sVertex outV;
        outV.pos = v.position;
        outV.color = XMFLOAT4(1.f, 1.f, 0.f, 1.f); // Yellow cylinder
        vertecies.push_back(outV);
    }

    // Convert cylinder indices
    std::vector<uint16_t> indices = cylinder.GetIndices16();

    // ---- Add Cube ----
    cMeshGenerator::sMeshData cube = meshGenerator.CreateCube();

    UINT cubeVertexOffset = static_cast<UINT>(vertecies.size());
    UINT cubeIndexOffset = static_cast<UINT>(indices.size());

    sSubmeshGeometry cubeSubmesh;
    cubeSubmesh.indexCount = static_cast<UINT>(cube.indices32.size());
    cubeSubmesh.startIndexLocation = cubeIndexOffset;
    cubeSubmesh.startVertexLocation = cubeVertexOffset;

    // Add cube vertices
    for (const auto& v : cube.vertecies)
    {
        sVertex outV;
        outV.pos = v.position;
        outV.color = XMFLOAT4(0.2f, 0.6f, 1.f, 1.f); // Blue cube
        vertecies.push_back(outV);
    }

    // Add cube indices
    auto cubeIndices16 = cube.GetIndices16();
    indices.insert(indices.end(), cubeIndices16.begin(), cubeIndices16.end());

    // ---- Add Sphere ----
    cMeshGenerator::sMeshData sphere = meshGenerator.CreateSphere(1.0f, 20, 20);

    UINT sphereVertexOffset = static_cast<UINT>(vertecies.size());
    UINT sphereIndexOffset = static_cast<UINT>(indices.size());

    sSubmeshGeometry sphereSubmesh;
    sphereSubmesh.indexCount = static_cast<UINT>(sphere.indices32.size());
    sphereSubmesh.startIndexLocation = sphereIndexOffset;
    sphereSubmesh.startVertexLocation = sphereVertexOffset;

    // Add sphere vertices
    for (const auto& v : sphere.vertecies)
    {
        sVertex outV;
        outV.pos = v.position;
        outV.color = XMFLOAT4(1.f, 0.4f, 0.4f, 1.f); // Reddish sphere
        vertecies.push_back(outV);
    }

    // Add sphere indices
    auto sphereIndices16 = sphere.GetIndices16();
    indices.insert(indices.end(), sphereIndices16.begin(), sphereIndices16.end());

    // ---- Create Geometry Buffers ----
    const UINT vbByteSize = static_cast<UINT>(vertecies.size() * sizeof(sVertex));
    const UINT ibByteSize = static_cast<UINT>(indices.size() * sizeof(uint16_t));

    sMeshGeometry* geo = new sMeshGeometry();
    geo->name = "shapeGeo";

    cDirectX12Util::ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->vertexBufferCPU));
    CopyMemory(geo->vertexBufferCPU->GetBufferPointer(), vertecies.data(), vbByteSize);

    cDirectX12Util::ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->indexBufferCPU));
    CopyMemory(geo->indexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    geo->vertexBufferGPU = cDirectX12Util::CreateDefaultBuffer(
        m_pDeviceManager->GetDevice(),
        m_pDeviceManager->GetCommandList(),
        vertecies.data(),
        vbByteSize,
        geo->vertexBufferUploader
    );

    geo->indexBufferGPU = cDirectX12Util::CreateDefaultBuffer(
        m_pDeviceManager->GetDevice(),
        m_pDeviceManager->GetCommandList(),
        indices.data(),
        ibByteSize,
        geo->indexBufferUploader
    );

    geo->vertexByteStride = sizeof(sVertex);
    geo->vertexBufferByteSize = vbByteSize;
    geo->indexFormat = DXGI_FORMAT_R16_UINT;
    geo->indexBufferByteSize = ibByteSize;

    geo->drawArguments["cylinder"] = cylinderSubmesh;
    geo->drawArguments["cube"] = cubeSubmesh;
    geo->drawArguments["sphere"] = sphereSubmesh;

    // Store geometry
    m_geometries[geo->name] = geo;

    // ---- Create Render Item: Cylinder ----
    sRenderItem* pItem2 = new sRenderItem();
    pItem2->pGeometry = geo;
    pItem2->objCBIndex = 0;

    float scale = 2.f;
    XMMATRIX scaleMatrix = XMMatrixScaling(scale, scale, scale);
    XMMATRIX worldMatrix = scaleMatrix * XMMatrixTranslation(-3.f, 0.f, 0.f);
    XMStoreFloat4x4(&pItem2->worldMatrix, worldMatrix);

    sSubmeshGeometry submeshCyl = geo->drawArguments["cylinder"];
    pItem2->indexCount = submeshCyl.indexCount;
    pItem2->startIndexLocation = submeshCyl.startIndexLocation;
    pItem2->baseVertexLocation = submeshCyl.startVertexLocation;

    m_renderItems.push_back(pItem2);

    // ---- Create Render Item: Cube ----
    sRenderItem* pItem3 = new sRenderItem();
    pItem3->pGeometry = geo;
    pItem3->objCBIndex = 1;

    XMMATRIX cubeScale = XMMatrixScaling(2.f, 2.f, 2.f);
    XMMATRIX cubeWorld = cubeScale * XMMatrixTranslation(3.f, 0.f, 0.f);
    XMStoreFloat4x4(&pItem3->worldMatrix, cubeWorld);

    sSubmeshGeometry submeshCube = geo->drawArguments["cube"];
    pItem3->indexCount = submeshCube.indexCount;
    pItem3->startIndexLocation = submeshCube.startIndexLocation;
    pItem3->baseVertexLocation = submeshCube.startVertexLocation;

    m_renderItems.push_back(pItem3);

    // ---- Create Render Item: Sphere ----
    sRenderItem* pItem4 = new sRenderItem();
    pItem4->pGeometry = geo;
    pItem4->objCBIndex = 2;

    XMMATRIX sphereScale = XMMatrixScaling(2.f, 2.f, 2.f);
    XMMATRIX sphereWorld = sphereScale * XMMatrixTranslation(0.f, 0.f, 3.f);
    XMStoreFloat4x4(&pItem4->worldMatrix, sphereWorld);

    sSubmeshGeometry submeshSphere = geo->drawArguments["sphere"];
    pItem4->indexCount = submeshSphere.indexCount;
    pItem4->startIndexLocation = submeshSphere.startIndexLocation;
    pItem4->baseVertexLocation = submeshSphere.startVertexLocation;

    m_renderItems.push_back(pItem4);
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::Update(XMMATRIX _view)
{
    // Advance frame resource
    m_currentFrameResourceIndex = (m_currentFrameResourceIndex + 1) % c_NumberOfFrameResources;
    m_pCurrentFrameResource = m_frameResources[m_currentFrameResourceIndex];
    WaitForCurrentFrameResourceIfInUse();

    // Reset command list & allocator
    ID3D12PipelineState* pPso = m_pPipelineManager->GetPipelineStateObject();
    ID3D12CommandAllocator* pDirectCmdListAlloc = m_pCurrentFrameResource->pCmdListAlloc.Get();
    ID3D12GraphicsCommandList* pCommandList = m_pDeviceManager->GetCommandList();

    cDirectX12Util::ThrowIfFailed(pDirectCmdListAlloc->Reset());
    cDirectX12Util::ThrowIfFailed(pCommandList->Reset(pDirectCmdListAlloc, pPso));

    // Handle window resize
    if (m_pWindow->GetHasResized())
    {
        OnResize();
        m_pWindow->SetHasResized(false);
    }

    XMVECTOR eyePos = XMVectorSet(0.0f, 10.0f, -20.0f, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(eyePos, target, up);
    XMStoreFloat4x4(&m_view, view);

    // === Upload data to GPU buffers ===
    UpdateObjectCB();  // Writes m_renderItems[*]->worldMatrix to per-object CB
    UpdatePassCB();    // Writes m_mainPassCB to per-pass CB
}

// --------------------------------------------------------------------------------------------------------------------------
// clears backbuffer, depthstencil and presents the frame to the screen

void cDirectX12::Draw()
{
    ID3D12CommandAllocator*     pDirectCmdListAlloc = m_pCurrentFrameResource->pCmdListAlloc.Get();
    ID3D12GraphicsCommandList*  pCommandList        = m_pDeviceManager->GetCommandList();
    ID3D12CommandQueue*         pCommandQueue       = m_pDeviceManager->GetCommandQueue();
    ID3D12PipelineState*        pPso                = m_pPipelineManager->GetPipelineStateObject();
    ID3D12RootSignature*        pRootSignature      = m_pPipelineManager->GetRootSignature();
    IDXGISwapChain4*            pSwapChain          = m_pSwapChainManager->GetSwapChain();
    D3D12_VIEWPORT&             rViewport           = m_pSwapChainManager->GetViewport();
    ID3D12DescriptorHeap*       pCbvHeap            = m_pBufferManager->GetCbvHeap();
    ID3D12Fence*                pFence              = m_pDeviceManager->GetFence();

    // === Set viewport and scissor ===
    pCommandList->RSSetViewports(1, &rViewport);

    D3D12_RECT scissorRect = { 0, 0, m_pWindow->GetWidth(), m_pWindow->GetHeight() };
    pCommandList->RSSetScissorRects(1, &scissorRect);

    // === Transition back buffer from PRESENT to RENDER_TARGET ===
    pCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
            m_pSwapChainManager->GetCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        )
    );

    // === Clear render target ===
    float clearColor[] = { 0.f, 0.f, 0.f, 1.f }; // Black
    pCommandList->ClearRenderTargetView(m_pSwapChainManager->GetCurrentBackBufferView(), clearColor, 0, nullptr);

    // === Clear depth/stencil buffer ===
    pCommandList->ClearDepthStencilView(
        m_pSwapChainManager->GetDepthStencilView(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f, 0,
        0, nullptr
    );

    // === Set render targets ===
    pCommandList->OMSetRenderTargets(
        1,
        &m_pSwapChainManager->GetCurrentBackBufferView(),
        TRUE,
        &m_pSwapChainManager->GetDepthStencilView()
    );

    // === Bind descriptor heap ===
    ID3D12DescriptorHeap* descriptorHeaps[] = { pCbvHeap };
    pCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // === Set root signature ===
    pCommandList->SetGraphicsRootSignature(pRootSignature);

    // === Compute per-frame descriptor offset ===
    UINT descriptorsPerFrame    = static_cast<UINT>(m_renderItems.size()) + 1; // +1 for pass CBV
    UINT baseOffset             = m_currentFrameResourceIndex * descriptorsPerFrame;
    UINT descriptorSize         = m_pDeviceManager->GetDescriptorSizes().cbvSrvUav;

    // === Set Pass CBV (root param 1, register b1) ===
    CD3DX12_GPU_DESCRIPTOR_HANDLE passCbvHandle(pCbvHeap->GetGPUDescriptorHandleForHeapStart());
    passCbvHandle.Offset(baseOffset + m_renderItems.size(), descriptorSize); // Pass CBV is last
    pCommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

    // === Draw render items ===
    for (sRenderItem* renderItem : m_renderItems)
    {
        // Set vertex/index buffers and primitive topology
        pCommandList->IASetVertexBuffers(0, 1, &renderItem->pGeometry->GetVertexBufferView());
        pCommandList->IASetIndexBuffer(&renderItem->pGeometry->GetIndexBufferView());
        pCommandList->IASetPrimitiveTopology(renderItem->primitiveType);

        // === Set Object CBV (root param 0, register b0) ===
        CD3DX12_GPU_DESCRIPTOR_HANDLE objCbvHandle(pCbvHeap->GetGPUDescriptorHandleForHeapStart());
        objCbvHandle.Offset(baseOffset + renderItem->objCBIndex, descriptorSize); // Per-frame offset
        pCommandList->SetGraphicsRootDescriptorTable(0, objCbvHandle);

        // === Draw the object ===
        pCommandList->DrawIndexedInstanced(
            renderItem->indexCount,
            1,
            renderItem->startIndexLocation,
            renderItem->baseVertexLocation,
            0
        );
    }

    // === Transition back buffer from RENDER_TARGET to PRESENT ===
    pCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
            m_pSwapChainManager->GetCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        )
    );

    // === Close and execute command list ===
    cDirectX12Util::ThrowIfFailed(pCommandList->Close());

    ID3D12CommandList* cmdLists[] = { pCommandList };
    pCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // === Signal fence for current frame ===
    UINT64 currentFence = m_pDeviceManager->GetFenceValue() + 1;
    pCommandQueue->Signal(pFence, currentFence);
    m_pDeviceManager->SetFenceValue(currentFence);
    m_pCurrentFrameResource->fence = currentFence;

    // === Present frame ===
    cDirectX12Util::ThrowIfFailed(pSwapChain->Present(1, 0));
}


// --------------------------------------------------------------------------------------------------------------------------
// calculates the aspectratio of the window 

float cDirectX12::GetAspectRatio() const
{
    return static_cast<float>(m_pWindow->GetWidth())  / static_cast<float>(m_pWindow->GetHeight());
}


// --------------------------------------------------------------------------------------------------------------------------
// computes fps and time to render one frame

void cDirectX12::CalculateFrameStats() const
{
    static int frameCnt = 0;
    static float timeElapsed = 0.f; 

    frameCnt++; 

    if ((m_pTimer->GetTotalTime() - timeElapsed) >= 1.f)
    {
        int fps = frameCnt;
        float mspf = 1000.f / fps;

        std::cout << "fps: " << fps << ", mspf: " << mspf << "\n";

        frameCnt = 0;
        timeElapsed += 1.f;
    }

}

// --------------------------------------------------------------------------------------------------------------------------
// handles the resize
// resizes swapchain buffers and recreates render target view and depth stencil view

void cDirectX12::OnResize()
{
    WaitForGPU();

    m_pSwapChainManager->OnResize();

    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * c_pi, GetAspectRatio(), 0.1f, 1000.0f);
    XMStoreFloat4x4(&m_proj, P);
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::UpdateObjectCB()
{
    auto currObjCB = m_pCurrentFrameResource->pObjectCB;

    for (sRenderItem* pItem : m_renderItems)
    {
        if (pItem->numberOfFramesDirty > 0)
        {
            XMMATRIX world = XMLoadFloat4x4(&pItem->worldMatrix);

            sObjectConstants objConstants; 
            XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(world));

            currObjCB->CopyData(pItem->objCBIndex, objConstants);

            pItem->numberOfFramesDirty--;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::UpdatePassCB()
{
    
    XMMATRIX view           = XMLoadFloat4x4(&m_view);
    XMMATRIX proj           = XMLoadFloat4x4(&m_proj);
    XMMATRIX viewProj       = XMMatrixMultiply(view, proj);
    XMMATRIX invView        = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj        = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj    = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    sPassConstants passConstants;

    XMStoreFloat4x4(&passConstants.view,        XMMatrixTranspose(view));
    XMStoreFloat4x4(&passConstants.invView,     XMMatrixTranspose(invView));
    XMStoreFloat4x4(&passConstants.proj,        XMMatrixTranspose(proj));
    XMStoreFloat4x4(&passConstants.invProj,     XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&passConstants.viewProj,    XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&passConstants.invViewProj, XMMatrixTranspose(invViewProj));

    // todo correct values for eyepos
    passConstants.eyePos                = XMFLOAT3(0.0f, 0.0f, -10.0f);
    passConstants.renderTargetSize      = XMFLOAT2((float)m_pWindow->GetWidth() , (float)m_pWindow->GetHeight());
    passConstants.invRenderTargetSize   = XMFLOAT2(1.0f / m_pWindow->GetWidth(), 1.0f / m_pWindow->GetHeight());
    passConstants.nearZ                 = 1.0f;
    passConstants.farZ                  = 1000.0f;
    passConstants.totalTime             = m_pTimer->GetTotalTime();
    passConstants.deltaTime             = m_pTimer->GetDeltaTime();

    auto currPassCB = m_pCurrentFrameResource->pPassCB;

    currPassCB->CopyData(0, passConstants);
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::InitializeFrameResources()
{
    ID3D12Device*           pDevice     = m_pDeviceManager->GetDevice();
    ID3D12DescriptorHeap*   pCbvHeap    = m_pBufferManager->GetCbvHeap();

    for (int index = 0; index < c_NumberOfFrameResources; index++)
    {
        std::cout << m_renderItems.size() << std::endl; 
        m_frameResources.push_back(new sFrameResource(m_pDeviceManager->GetDevice(), 1, m_renderItems.size()));
    }

    UINT descriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = pCbvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT frameIndex = 0; frameIndex < c_NumberOfFrameResources; frameIndex++)
    {
        sFrameResource* frameResource = m_frameResources[frameIndex];

        for (UINT objIndex = 0; objIndex < m_renderItems.size(); objIndex++)
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation  = frameResource->pObjectCB->GetResource()->GetGPUVirtualAddress() + objIndex * frameResource->pObjectCB->GetElementByteSize();
            cbvDesc.SizeInBytes     = frameResource->pObjectCB->GetElementByteSize();

            pDevice->CreateConstantBufferView(
                &cbvDesc,
                cbvHandle);

            cbvHandle.ptr += descriptorSize;
        }

        // Create pass CBV
        D3D12_CONSTANT_BUFFER_VIEW_DESC passCbvDesc = {};

        passCbvDesc.BufferLocation  = frameResource->pPassCB->GetResource()->GetGPUVirtualAddress();
        passCbvDesc.SizeInBytes     = frameResource->pPassCB->GetElementByteSize();

        pDevice->CreateConstantBufferView(&passCbvDesc, cbvHandle);

        cbvHandle.ptr += descriptorSize;
    }
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::WaitForCurrentFrameResourceIfInUse()
{
    UINT64 gpuCompletedFence = m_pDeviceManager->GetFence()->GetCompletedValue();

    // Only wait if the current frame resource has a fence set and GPU hasn't reached it yet
    if (m_pCurrentFrameResource->fence != 0 &&
        gpuCompletedFence < m_pCurrentFrameResource->fence)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        if (eventHandle == nullptr)
        {
            throw std::runtime_error("Failed to create event handle.");
        }

        cDirectX12Util::ThrowIfFailed(m_pDeviceManager->GetFence()->SetEventOnCompletion(m_pCurrentFrameResource->fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::WaitForGPU()
{
    UINT64              fenceValue      = m_pDeviceManager->GetFenceValue() + 1;
    ID3D12CommandQueue* pCommandQueue   = m_pDeviceManager->GetCommandQueue();
    ID3D12Fence*        pFence          = m_pDeviceManager->GetFence();

    // Signal the fence
    cDirectX12Util::ThrowIfFailed(pCommandQueue->Signal(pFence, fenceValue));
    m_pDeviceManager->SetFenceValue(fenceValue);

    // Wait until GPU reaches this point
    if (pFence->GetCompletedValue() < fenceValue)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        if (eventHandle == nullptr)
            throw std::runtime_error("Failed to create event handle.");

        pFence->SetEventOnCompletion(fenceValue, eventHandle);
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------------
