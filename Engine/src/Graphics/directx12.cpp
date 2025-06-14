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

void cDirectX12::Initialize(cWindow* _pWindow, cTimer* _pTimer, unsigned int _maxNumberOfRenderItems)
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
    m_maxNumberOfRenderItems = _maxNumberOfRenderItems; 

    m_pDeviceManager = new cDeviceManager();
    m_pDeviceManager->Initialize();

    m_pSwapChainManager = new cSwapChainManager(m_pDeviceManager, m_pWindow);
    m_pSwapChainManager->Initialize();

    m_pBufferManager = new cBufferManager(m_pDeviceManager, m_pSwapChainManager);
    m_pBufferManager->Initialize(_maxNumberOfRenderItems);

    m_pPipelineManager = new cPipelineManager(m_pDeviceManager);
    m_pPipelineManager->Initialize(); 
    
    m_pGeometry = new sMeshGeometry;

    InitializeFrameResources();

   
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::Finalize()
{
    WaitForGPU();

    for (auto* frameResource : m_frameResources)
    {
        delete frameResource;
    }

    for (auto& pair : m_geometries)
    {
        delete pair.second;
    }
    
    delete m_pGeometry;

    delete m_pBufferManager;
    delete m_pSwapChainManager;
    delete m_pDeviceManager;
    delete m_pPipelineManager;
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::InitializeMesh(cMeshGenerator::sMeshData& _rMeshData, std::string& _rName, XMFLOAT4 _rColor)
{

    sSubmeshGeometry subMesh;

    subMesh.indexCount              = _rMeshData.indices32.size();
    subMesh.startIndexLocation      = m_indices.size();
    subMesh.startVertexLocation     = m_vertecis.size();

    // todo: rename vertecies 
    for (auto v : _rMeshData.vertecies)
    {
        sVertex vOut; 

        vOut.pos = v.position;
        vOut.color = _rColor;
        m_vertecis.push_back(vOut);
    }

    
    std::vector<uint16> meshIndices16 = _rMeshData.GetIndices16(); 
    m_indices.insert(m_indices.end(), meshIndices16.begin(), meshIndices16.end());

    m_pGeometry->drawArguments[_rName] = subMesh;
}

// --------------------------------------------------------------------------------------------------------------------------

sMeshGeometry* cDirectX12::InitializeGeometryBuffer()
{
    const UINT vbByteSize = static_cast<UINT>(m_vertecis.size() * sizeof(sVertex));
    const UINT ibByteSize = static_cast<UINT>(m_indices.size() * sizeof(uint16_t));

    m_pGeometry->name = "shapeGeo";

    cDirectX12Util::ThrowIfFailed(D3DCreateBlob(vbByteSize, &m_pGeometry->vertexBufferCPU));
    std::cout << m_vertecis[100].pos.z  << std::endl;
    CopyMemory(m_pGeometry->vertexBufferCPU->GetBufferPointer(), m_vertecis.data(), vbByteSize);

    cDirectX12Util::ThrowIfFailed(D3DCreateBlob(ibByteSize, &m_pGeometry->indexBufferCPU));
    CopyMemory(m_pGeometry->indexBufferCPU->GetBufferPointer(), m_indices.data(), ibByteSize);

    m_pGeometry->vertexBufferGPU = cDirectX12Util::CreateDefaultBuffer(
        m_pDeviceManager->GetDevice(),
        m_pDeviceManager->GetCommandList(),
        m_vertecis.data(),
        vbByteSize,
        m_pGeometry->vertexBufferUploader
    );

    m_pGeometry->indexBufferGPU = cDirectX12Util::CreateDefaultBuffer(
        m_pDeviceManager->GetDevice(),
        m_pDeviceManager->GetCommandList(),
        m_indices.data(),
        ibByteSize,
        m_pGeometry->indexBufferUploader
    );

    m_pGeometry->vertexByteStride = sizeof(sVertex);
    m_pGeometry->vertexBufferByteSize = vbByteSize;
    m_pGeometry->indexFormat = DXGI_FORMAT_R16_UINT;
    m_pGeometry->indexBufferByteSize = ibByteSize;

    m_pDeviceManager->GetCommandList()->Close();
    ID3D12CommandList* cmdLists[] = { m_pDeviceManager->GetCommandList() };
    m_pDeviceManager->GetCommandQueue()->ExecuteCommandLists(_countof(cmdLists), cmdLists);
    m_pDeviceManager->FlushCommandQueue();

    return m_pGeometry;
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::Update(XMMATRIX _view, std::array<sRenderItem, 1000>* _pRenderItems)
{
    m_pRenderItems = _pRenderItems; 
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

    XMVECTOR eyePos = XMVectorSet(0.0f, 10.0f, -100.0f, 1.0f);
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
    UINT descriptorsPerFrame    = static_cast<UINT>(m_maxNumberOfRenderItems) + 1; // +1 for pass CBV
    UINT baseOffset             = m_currentFrameResourceIndex * descriptorsPerFrame;
    UINT descriptorSize         = m_pDeviceManager->GetDescriptorSizes().cbvSrvUav;

    // === Set Pass CBV (root param 1, register b1) ===
    CD3DX12_GPU_DESCRIPTOR_HANDLE passCbvHandle(pCbvHeap->GetGPUDescriptorHandleForHeapStart());
    passCbvHandle.Offset(baseOffset + m_maxNumberOfRenderItems, descriptorSize); // Pass CBV is last
    pCommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

    for (auto& renderItem : *m_pRenderItems) 
    {
        // Set vertex/index buffers and primitive topology
        pCommandList->IASetVertexBuffers(0, 1, &renderItem.pGeometry->GetVertexBufferView());
        pCommandList->IASetIndexBuffer(&renderItem.pGeometry->GetIndexBufferView());
        pCommandList->IASetPrimitiveTopology(renderItem.primitiveType);

        // === Set Oject CBV (root param 0, register b0) ===
        CD3DX12_GPU_DESCRIPTOR_HANDLE objCbvHandle(pCbvHeap->GetGPUDescriptorHandleForHeapStart());
        objCbvHandle.Offset(baseOffset + renderItem.objCBIndex, descriptorSize); // Per-frame offset
        pCommandList->SetGraphicsRootDescriptorTable(0, objCbvHandle);

        // === Draw the object ===
        pCommandList->DrawIndexedInstanced(
            renderItem.indexCount,
            1,
            renderItem.startIndexLocation,
            renderItem.baseVertexLocation,
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

    for (sRenderItem& pItem : *m_pRenderItems)
    {
        if (pItem.numberOfFramesDirty > 0)
        {
            XMMATRIX world = XMLoadFloat4x4(&pItem.worldMatrix);

            sObjectConstants objConstants; 
            XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(world));

            currObjCB->CopyData(pItem.objCBIndex, objConstants);

            pItem.numberOfFramesDirty--;
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
        std::cout << m_maxNumberOfRenderItems << std::endl;
        m_frameResources.push_back(new sFrameResource(m_pDeviceManager->GetDevice(), 1, m_maxNumberOfRenderItems));
    }

    UINT descriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = pCbvHeap->GetCPUDescriptorHandleForHeapStart();

    for (UINT frameIndex = 0; frameIndex < c_NumberOfFrameResources; frameIndex++)
    {
        sFrameResource* frameResource = m_frameResources[frameIndex];

        for (UINT objIndex = 0; objIndex < m_maxNumberOfRenderItems; objIndex++)
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
