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
    
    InitializeFrameResources();
    InitializeVertices();

    m_pDeviceManager->GetCommandList()->Close();
    ID3D12CommandList* cmdLists[] = { m_pDeviceManager->GetCommandList() };
    m_pDeviceManager->GetCommandQueue()->ExecuteCommandLists(_countof(cmdLists), cmdLists);
    m_pDeviceManager->FlushCommandQueue();
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::Finalize()
{
    WaitForGPU();

    for (auto* frameResource : m_frameResources)
    {
        delete frameResource;
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
	// vertices	

    std::array<sVertex, 8> vertices =
	{
		sVertex{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)   },
        sVertex{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black)   },
        sVertex{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red)     },
        sVertex{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)   },
        sVertex{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue)    },
        sVertex{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow)  },
        sVertex{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan)    },
        sVertex{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) }
	};

    std::array<std::uint16_t, 36> indices =
    {
        // front face
        0, 1, 2,
        0, 2, 3,
        // back face
        4, 6, 5,
        4, 7, 6,
        // left face
        4, 5, 1,
        4, 1, 0,
        // right face
        3, 2, 6,
        3, 6, 7,
        // top face
        1, 5, 6,
        1, 6, 2,
        // bottom face
        4, 0, 3,
        4, 3, 7
    };
    const UINT64 vbByteSize = static_cast<UINT>(vertices.size() * sizeof(sVertex));
    const UINT ibByteSize = static_cast<UINT>(indices.size() * sizeof(std::uint16_t));

    m_pBoxGeometry = new sMeshGeometry();

    // Create CPU buffers (no need for SetName here)
    cDirectX12Util::ThrowIfFailed(D3DCreateBlob(vbByteSize, &m_pBoxGeometry->vertexBufferCPU));
    CopyMemory(m_pBoxGeometry->vertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    cDirectX12Util::ThrowIfFailed(D3DCreateBlob(ibByteSize, &m_pBoxGeometry->indexBufferCPU));
    CopyMemory(m_pBoxGeometry->indexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    // Create and name GPU buffers
    m_pBoxGeometry->vertexBufferGPU = cDirectX12Util::CreateDefaultBuffer(
        m_pDeviceManager->GetDevice(),
        m_pDeviceManager->GetCommandList(),
        vertices.data(),
        vbByteSize,
        m_pBoxGeometry->vertexBufferUploader
    );
    m_pBoxGeometry->vertexBufferGPU->SetName(L"Box_VertexBuffer_GPU");

    m_pBoxGeometry->indexBufferGPU = cDirectX12Util::CreateDefaultBuffer(
        m_pDeviceManager->GetDevice(),
        m_pDeviceManager->GetCommandList(),
        indices.data(),
        ibByteSize,
        m_pBoxGeometry->indexBufferUploader
    );
    m_pBoxGeometry->indexBufferGPU->SetName(L"Box_IndexBuffer_GPU");

    // Ensure the upload buffers are initialized before setting names
    if (m_pBoxGeometry->vertexBufferUploader != nullptr) 
    {
        m_pBoxGeometry->vertexBufferUploader->SetName(L"Box_VertexBuffer_Uploader");
    }
    else 
    {
        std::cerr << "Error: vertexBufferUploader is nullptr." << std::endl;
    }

    if (m_pBoxGeometry->indexBufferUploader != nullptr) 
    {
        m_pBoxGeometry->indexBufferUploader->SetName(L"Box_IndexBuffer_Uploader");
    }
    else 
    {
        std::cerr << "Error: indexBufferUploader is nullptr." << std::endl;
    }

    // Set up draw arguments and other properties
    m_pBoxGeometry->vertexByteStride = sizeof(sVertex);
    m_pBoxGeometry->vertexBufferByteSize = vbByteSize;
    m_pBoxGeometry->indexFormat = DXGI_FORMAT_R16_UINT;
    m_pBoxGeometry->indexBufferByteSize = ibByteSize;

    sSubmeshGeometry submesh;
    submesh.indexCount = (UINT)indices.size();
    submesh.startIndexLocation = 0;
    submesh.startVertexLocation = 0;
    m_pBoxGeometry->drawArguments["box"] = submesh;

    // Set world matrix transformation
    XMMATRIX worldMatrix = XMMatrixIdentity();

    // Define position
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    // Define scale
    float scale = 1.0f;

    // Create scaling and translation matrices
    XMMATRIX scaleMatrix = XMMatrixScaling(scale, scale, scale);
    XMMATRIX translationMatrix = XMMatrixTranslation(x, y, z);

    // Combine transformations: scale first, then translate
    worldMatrix = scaleMatrix * translationMatrix;

    // Convert the XMMATRIX to XMFLOAT4X4 for storage
    XMStoreFloat4x4(&m_world, worldMatrix);


}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::Update(XMMATRIX view)
{
    m_currentFrameResourceIndex = (m_currentFrameResourceIndex + 1) % c_NumberOfFrameResources;
    m_pCurrentFrameResource = m_frameResources[m_currentFrameResourceIndex];
    WaitForCurrentFrameResourceIfInUse();
    
    ID3D12PipelineState*        pPso                = m_pPipelineManager->GetPipelineStateObject();
    ID3D12CommandAllocator*     pDirectCmdListAlloc = m_pCurrentFrameResource->pCmdListAlloc.Get();
    ID3D12GraphicsCommandList*  pCommandList        = m_pDeviceManager->GetCommandList();

    cDirectX12Util::ThrowIfFailed(pDirectCmdListAlloc->Reset());
    cDirectX12Util::ThrowIfFailed(pCommandList->Reset(pDirectCmdListAlloc, pPso)); 

    if (m_pWindow->GetHasResized())
    {
        OnResize();
        m_pWindow->SetHasResized(false);
    }

    // Load matrices
    XMMATRIX world = XMLoadFloat4x4(&m_world);
    XMMATRIX proj = XMLoadFloat4x4(&m_proj);

    // Combine matrices into WorldViewProjection
    XMMATRIX worldViewProj = XMMatrixMultiply(world, view);
    worldViewProj = XMMatrixMultiply(worldViewProj, proj);

    // Transpose for HLSL (row-major expected)
    XMMATRIX worldViewProjT = XMMatrixTranspose(worldViewProj);

    // Pack into constant buffer struct
    sObjectConstants objConstants;
    XMStoreFloat4x4(&objConstants.world, worldViewProjT);

    // Upload to mapped constant buffer (frame index 0 for now)
    m_pBufferManager->GetObjectCB()->CopyData(0, objConstants);
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
    

    // === Reset allocator & command list BEFORE recording ===
   

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
    float clearColor[] = { 0.f, 0.f, 0.f, 1.f }; // Magenta
    pCommandList->ClearRenderTargetView(m_pSwapChainManager->GetCurrentBackBufferView(), clearColor, 0, nullptr);

    // === Clear depth/stencil buffer ===
    pCommandList->ClearDepthStencilView(
        m_pSwapChainManager->GetDepthStencilView(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f, 0,
        0, nullptr
    );

    // === Set render targets ===
    pCommandList->OMSetRenderTargets(1, &m_pSwapChainManager->GetCurrentBackBufferView(), TRUE, &m_pSwapChainManager->GetDepthStencilView());

    // === Bind descriptor heap ===
    ID3D12DescriptorHeap* descriptorHeaps[] = { pCbvHeap };
    pCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // === Set root signature ===
    pCommandList->SetGraphicsRootSignature(pRootSignature);

    // === Input assembler: vertex/index buffers ===
    pCommandList->IASetVertexBuffers(0, 1, &m_pBoxGeometry->GetVertexBufferView());
    pCommandList->IASetIndexBuffer(&m_pBoxGeometry->GetIndexBufferView());
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // === Set constant buffer view (CBV) ===
    pCommandList->SetGraphicsRootDescriptorTable(0, pCbvHeap->GetGPUDescriptorHandleForHeapStart());

    // === Draw geometry ===
    pCommandList->DrawIndexedInstanced(
        m_pBoxGeometry->drawArguments["box"].indexCount,
        1, 0, 0, 0
    );

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

    // Execute commands
    ID3D12CommandList* cmdLists[] = { pCommandList };
    pCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // Signal the fence
    UINT64 currentFence = m_pDeviceManager->GetFenceValue() + 1;
    pCommandQueue->Signal(pFence, currentFence);
    m_pDeviceManager->SetFenceValue(currentFence);
    m_pCurrentFrameResource->fence = currentFence;

    // THEN present
    cDirectX12Util::ThrowIfFailed(pSwapChain->Present(0, 0));

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

    for (auto& item : m_renderItems)
    {
        if (item->numberOfFramesDirty > 0)
        {
            XMMATRIX world = XMLoadFloat4x4(&item->worldMatrix);

            sObjectConstants objConstants; 
            XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(world));

            currObjCB->CopyData(item->objCBIndex, objConstants); 

            item->numberOfFramesDirty--; 
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::UpdatePassCB()
{
    
    XMMATRIX view = XMLoadFloat4x4(&m_view);
    XMMATRIX proj = XMLoadFloat4x4(&m_proj);
    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
    XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
    XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

    XMStoreFloat4x4(&m_passConstants.view, XMMatrixTranspose(view));
    XMStoreFloat4x4(&m_passConstants.invView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&m_passConstants.proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&m_passConstants.viewProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&m_passConstants.invViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&m_passConstants.eyePos, XMMatrixTranspose(invViewProj));

    // todo eye missing
    m_passConstants.renderTargetSize = XMFLOAT2((float)m_pWindow->GetWidth() , (float)m_pWindow->GetHeight());
    m_passConstants.invRenderTargetSize = XMFLOAT2(1.0f / m_pWindow->GetWidth(), 1.0f / m_pWindow->GetWidth());
    m_passConstants.nearZ = 1.0f;
    m_passConstants.farZ = 1000.0f;
    m_passConstants.totalTime = m_pTimer->GetTotalTime();
    m_passConstants.deltaTime = m_pTimer->GetDeltaTime();

    auto currPassCB = m_pCurrentFrameResource->pPassCB;

    currPassCB->CopyData(0, m_passConstants);
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::InitializeFrameResources()
{
    for (int index = 0; index < c_NumberOfFrameResources; index++)
    {
        m_frameResources.push_back(new sFrameResource(m_pDeviceManager->GetDevice(), 0, m_renderItems.size()));
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
    UINT64 fenceValue = m_pDeviceManager->GetFenceValue() + 1;

    ID3D12CommandQueue* pCommandQueue = m_pDeviceManager->GetCommandQueue();
    ID3D12Fence* pFence = m_pDeviceManager->GetFence();

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
