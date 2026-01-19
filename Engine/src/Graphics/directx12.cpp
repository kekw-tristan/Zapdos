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
#include "core/input.h"

#include "directx12Util.h"
#include "frameResource.h"
#include "swapChainManager.h"
#include "deviceManager.h"
#include "pipelineManager.h"
#include "bufferManager.h"
#include "renderItem.h"
#include "uploadBuffer.h"
#include "vertex.h"
#include "light.h"
#include "meshGeometry.h"
#include "meshGenerator.h"

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
        std::cout << "no pix installation found" << std::endl;
    }

    return pixInstallationPath / newestVersionFound / L"WinPixGpuCapturer.dll";
}

// --------------------------------------------------------------------------------------------------------------------------
// initializes all the directx12 components

void cDirectX12::Initialize(cWindow* _pWindow, cTimer* _pTimer, unsigned int _maxNumberOfRenderItems, unsigned int _maxNumberOfLights)
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
   
    m_pWindow                   = _pWindow;
    m_pTimer                    = _pTimer;
    m_maxNumberOfRenderItems    = _maxNumberOfRenderItems; 
    m_maxNumberOfLights         = _maxNumberOfLights;

    m_pDeviceManager    = new cDeviceManager();
    m_pSwapChainManager = new cSwapChainManager(m_pDeviceManager, m_pWindow);
    m_pBufferManager    = new cBufferManager(m_pDeviceManager, m_pSwapChainManager);
    m_pPipelineManager  = new cPipelineManager(m_pDeviceManager);
    m_pGeometry         = new sMeshGeometry;

    m_pDeviceManager    ->Initialize();
    m_pSwapChainManager ->Initialize();
    m_pBufferManager    ->Initialize(_maxNumberOfRenderItems, _maxNumberOfLights);
    m_pPipelineManager  ->Initialize(); 
    
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

    subMesh.indexCount              = _rMeshData.GetIndices16().size();
    subMesh.startIndexLocation      = m_indices.size();
    subMesh.startVertexLocation     = m_vertecis.size();

    for (auto v : _rMeshData.vertices)
    {
        sVertex vOut = {};

        vOut.position = v.position;
        vOut.normal = v.normal;
        vOut.tangentU = v.tangentU;
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

    m_pGeometry->vertexByteStride       = sizeof(sVertex);
    m_pGeometry->vertexBufferByteSize   = vbByteSize;
    m_pGeometry->indexFormat            = DXGI_FORMAT_R16_UINT;
    m_pGeometry->indexBufferByteSize    = ibByteSize;

    
    m_pDeviceManager->GetCommandList()->Close();
    ID3D12CommandList* cmdLists[] = { m_pDeviceManager->GetCommandList() };
    m_pDeviceManager->GetCommandQueue()->ExecuteCommandLists(_countof(cmdLists), cmdLists);
    m_pDeviceManager->FlushCommandQueue();
    
    return m_pGeometry;
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::Update(XMMATRIX _view, XMFLOAT3 _eyePos, std::vector<sRenderItem>* _pRenderItems, std::vector<sLightConstants>* _pLights)
{
    m_pRenderItems  = _pRenderItems; 
    m_pLights       = _pLights;
    m_eyePos        = _eyePos;

    // Advance frame resource
    m_currentFrameResourceIndex = (m_currentFrameResourceIndex + 1) % c_NumberOfFrameResources;
    m_pCurrentFrameResource     = m_frameResources[m_currentFrameResourceIndex];
    WaitForCurrentFrameResourceIfInUse();

    // Reset command list & allocator
    ID3D12PipelineState*        pPso                = m_pPipelineManager->GetPipelineStateObject();
    ID3D12CommandAllocator*     pDirectCmdListAlloc = m_pCurrentFrameResource->pCmdListAlloc.Get();
    ID3D12GraphicsCommandList*  pCommandList        = m_pDeviceManager->GetCommandList();

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
    XMVECTOR up     = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(eyePos, target, up);
    XMStoreFloat4x4(&m_view, _view);

    // === Upload data to GPU buffers ===
    UpdateObjectCB();  
    UpdatePassCB();   
    UpdateLightCB();
}

// --------------------------------------------------------------------------------------------------------------------------
// clears backbuffer, depthstencil and presents the frame to the screen
void cDirectX12::Draw()
{
    ID3D12CommandAllocator* pDirectCmdListAlloc = m_pCurrentFrameResource->pCmdListAlloc.Get();
    ID3D12GraphicsCommandList* pCommandList = m_pDeviceManager->GetCommandList();
    ID3D12CommandQueue* pCommandQueue = m_pDeviceManager->GetCommandQueue();
    ID3D12PipelineState* pPso = m_pPipelineManager->GetPipelineStateObject();
    ID3D12RootSignature* pRootSignature = m_pPipelineManager->GetRootSignature();
    IDXGISwapChain4* pSwapChain = m_pSwapChainManager->GetSwapChain();
    D3D12_VIEWPORT& rViewport = m_pSwapChainManager->GetViewport();
    ID3D12DescriptorHeap* pCbvHeap = m_pBufferManager->GetCbvHeap();
    ID3D12Fence* pFence = m_pDeviceManager->GetFence();

    // Set viewport and scissor
    pCommandList->RSSetViewports(1, &rViewport);

    D3D12_RECT scissorRect = { 0, 0, m_pWindow->GetWidth(), m_pWindow->GetHeight() };
    pCommandList->RSSetScissorRects(1, &scissorRect);

    // Transition back buffer PRESENT -> RENDER_TARGET
    pCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
            m_pSwapChainManager->GetCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        )
    );

    // Clear RTV and DSV
    float clearColor[] = { 0.f, 0.f, 0.f, 1.f };
    pCommandList->ClearRenderTargetView(m_pSwapChainManager->GetCurrentBackBufferView(), clearColor, 0, nullptr);
    pCommandList->ClearDepthStencilView(
        m_pSwapChainManager->GetDepthStencilView(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f, 0, 0, nullptr);

    // Set render targets
    pCommandList->OMSetRenderTargets(
        1, &m_pSwapChainManager->GetCurrentBackBufferView(), TRUE,
        &m_pSwapChainManager->GetDepthStencilView());

    

    // Bind descriptor heap
    ID3D12DescriptorHeap* descriptorHeaps[] = { pCbvHeap };
    pCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // Set root signature
    pCommandList->SetGraphicsRootSignature(pRootSignature);

    UINT descriptorSize = m_pDeviceManager->GetDescriptorSizes().cbvSrvUav;

    // Update descriptors per frame to include render items, pass CBV, and lights SRV
    UINT descriptorsPerFrame = m_maxNumberOfRenderItems + 1 + 1; // +1 for pass CBV, +1 for lights SRV
    UINT baseOffset = m_currentFrameResourceIndex * descriptorsPerFrame;

    // === Lights SRV (root param 2, t32) ===
    UINT lightIndex = baseOffset + m_maxNumberOfRenderItems + 1;
    CD3DX12_GPU_DESCRIPTOR_HANDLE lightSrvHandle(pCbvHeap->GetGPUDescriptorHandleForHeapStart());
    lightSrvHandle.Offset(lightIndex, descriptorSize);
    pCommandList->SetGraphicsRootDescriptorTable(2, lightSrvHandle);

    // === Pass CBV (root param 1, b1) ===
    UINT passIndex = baseOffset + m_maxNumberOfRenderItems;
    CD3DX12_GPU_DESCRIPTOR_HANDLE passCbvHandle(pCbvHeap->GetGPUDescriptorHandleForHeapStart());
    passCbvHandle.Offset(passIndex, descriptorSize);
    pCommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);

    // === Texturen (root param 3, t0..t6) ===
    UINT texBaseIndex = m_pBufferManager->GetTextureOffset();
    CD3DX12_GPU_DESCRIPTOR_HANDLE texHandle(pCbvHeap->GetGPUDescriptorHandleForHeapStart());
    texHandle.Offset(texBaseIndex, descriptorSize);
    pCommandList->SetGraphicsRootDescriptorTable(3, texHandle);

    // Draw all render items
    for (auto& renderItem : *m_pRenderItems)
    {
        pCommandList->IASetVertexBuffers(0, 1, &renderItem.pGeometry->GetVertexBufferView());
        pCommandList->IASetIndexBuffer(&renderItem.pGeometry->GetIndexBufferView());
        pCommandList->IASetPrimitiveTopology(renderItem.primitiveType);

        UINT objIndex = baseOffset + renderItem.objCBIndex;
        CD3DX12_GPU_DESCRIPTOR_HANDLE objCbvHandle(pCbvHeap->GetGPUDescriptorHandleForHeapStart());
        objCbvHandle.Offset(objIndex, descriptorSize);
        pCommandList->SetGraphicsRootDescriptorTable(0, objCbvHandle);


        pCommandList->DrawIndexedInstanced(
            renderItem.indexCount,
            1,
            renderItem.startIndexLocation,
            renderItem.baseVertexLocation,
            0);
    }

    // Transition back buffer RENDER_TARGET -> PRESENT
    pCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
            m_pSwapChainManager->GetCurrentBackBuffer(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        )
    );

    // Close and execute command list
    cDirectX12Util::ThrowIfFailed(pCommandList->Close());

    ID3D12CommandList* cmdLists[] = { pCommandList };
    pCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // Signal fence
    UINT64 currentFence = m_pDeviceManager->GetFenceValue() + 1;
    pCommandQueue->Signal(pFence, currentFence);
    m_pDeviceManager->SetFenceValue(currentFence);
    m_pCurrentFrameResource->fence = currentFence;

    // Present frame
    cDirectX12Util::ThrowIfFailed(pSwapChain->Present(0, 0));
}


// --------------------------------------------------------------------------------------------------------------------------

float cDirectX12::GetAspectRatio() const
{
    return static_cast<float>(m_pWindow->GetWidth())  / static_cast<float>(m_pWindow->GetHeight());
}


// --------------------------------------------------------------------------------------------------------------------------

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

void cDirectX12::OnResize()
{
    WaitForGPU();

    m_pSwapChainManager->OnResize();

    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * XM_PI, GetAspectRatio(), 0.1f, 1000.0f);
    XMStoreFloat4x4(&m_proj, P);
}

// --------------------------------------------------------------------------------------------------------------------------

sMeshGeometry* cDirectX12::GetGeometry()
{
    return m_pGeometry;
}

// --------------------------------------------------------------------------------------------------------------------------

ID3D12Device* cDirectX12::GetDevice()
{
    return m_pDeviceManager->GetDevice();
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::UpdateObjectCB()
{
    auto currObjCB = m_pCurrentFrameResource->pObjectCB;

    for (sRenderItem& pItem : *m_pRenderItems)
    {
        if (pItem.numberOfFramesDirty > 0)
        {
            sObjectConstants objConstants{};
            
                XMMATRIX world = XMLoadFloat4x4(&pItem.worldMatrix);
                XMStoreFloat4x4(&objConstants.world, XMMatrixTranspose(world));

                XMMATRIX worldNoTranslation = world;
                worldNoTranslation.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);
                XMStoreFloat4x4(&objConstants.worldInvTranspose, XMMatrixTranspose(XMMatrixInverse(nullptr, worldNoTranslation)));
            

            // Material
            if (pItem.pMaterial)
            {
                // Validate numbers
                auto& mat = *pItem.pMaterial;
                if (std::isfinite(mat.albedo.x) && std::isfinite(mat.albedo.y) && std::isfinite(mat.albedo.z) && std::isfinite(mat.alpha))
                    objConstants.baseColor = XMFLOAT4(mat.albedo.x, mat.albedo.y, mat.albedo.z, mat.alpha);
                else
                    objConstants.baseColor = XMFLOAT4(1, 1, 1, 1);

                objConstants.metallicFactor = std::isfinite(mat.metallic) ? mat.metallic : 0.f;
                objConstants.roughnessFactor = std::isfinite(mat.roughness) ? mat.roughness : 0.5f;
                objConstants.aoFactor = std::isfinite(mat.ao) ? mat.ao : 1.f;
                objConstants.emissive = mat.emissive; // optional: check finite
                objConstants.baseColorIndex = mat.baseColorIndex;
                std::cout << "baseColor index: " << objConstants.baseColorIndex << std::endl;
            }
            else
            {
                objConstants.baseColor = XMFLOAT4(1, 1, 1, 1);
                objConstants.metallicFactor = 0.f;
                objConstants.roughnessFactor = 0.5f;
                objConstants.aoFactor = 1.f;
                objConstants.emissive = XMFLOAT3(0, 0, 0);
                objConstants.baseColorIndex =0;
            }

            // Index safety
            if (pItem.objCBIndex >= 0 && pItem.objCBIndex < (int)m_maxNumberOfRenderItems)
            {
                currObjCB->CopyData(pItem.objCBIndex, objConstants);
            }
            else
            {
                // Debug warning
                OutputDebugString(L"Warning: objCBIndex out of bounds!\n");
            }

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

    passConstants.eyePos                = m_eyePos;
    passConstants.renderTargetSize      = XMFLOAT2((float)m_pWindow->GetWidth() , (float)m_pWindow->GetHeight());
    passConstants.invRenderTargetSize   = XMFLOAT2(1.0f / m_pWindow->GetWidth(), 1.0f / m_pWindow->GetHeight());
    passConstants.nearZ                 = 1.0f;
    passConstants.farZ                  = 1000.0f;
    passConstants.totalTime             = m_pTimer->GetTotalTime();
    passConstants.deltaTime             = m_pTimer->GetDeltaTime();

    passConstants.lightCount            = m_pLights->size(); 

    auto currPassCB = m_pCurrentFrameResource->pPassCB;

    currPassCB->CopyData(0, passConstants);
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::UpdateLightCB()
{
    auto currLightCB = m_pCurrentFrameResource->pLightBuffer;

    if (!m_pLights)
        return;

    for (size_t i = 0; i < m_pLights->size(); ++i)
    {
        currLightCB->CopyData(static_cast<int>(i), (*m_pLights)[i]);
    }
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::InitializeFrameResources()
{
    ID3D12Device* pDevice = m_pDeviceManager->GetDevice();
    ID3D12DescriptorHeap* pCbvHeap = m_pBufferManager->GetCbvHeap();

    UINT descriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = pCbvHeap->GetCPUDescriptorHandleForHeapStart();

    // Create frame resources
    for (int index = 0; index < c_NumberOfFrameResources; index++)
    {
        m_frameResources.push_back(
            new sFrameResource(
                pDevice,
                1,                        // pass count
                m_maxNumberOfRenderItems, // object count
                m_maxNumberOfLights       // light count
            )
        );
    }

    // Create CBVs for each frame resource
    for (UINT frameIndex = 0; frameIndex < c_NumberOfFrameResources; frameIndex++)
    {
        sFrameResource* frameResource = m_frameResources[frameIndex];

        UINT objCBByteSize      = cDirectX12Util::CalculateBufferByteSize(sizeof(sObjectConstants));
        UINT passCBByteSize     = cDirectX12Util::CalculateBufferByteSize(sizeof(sPassConstants));
        UINT lightCBByteSize    = cDirectX12Util::CalculateBufferByteSize(sizeof(sLightConstants));

        // === Object CBVs (b0) ===
        D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = frameResource->pObjectCB->GetResource()->GetGPUVirtualAddress();

        for (UINT objIndex = 0; objIndex < m_maxNumberOfRenderItems; objIndex++)
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = objCBAddress + objIndex * objCBByteSize;
            cbvDesc.SizeInBytes = objCBByteSize;

            pDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);
            cbvHandle.ptr += descriptorSize;
        }

        // === Pass CBV (b1) ===
        D3D12_CONSTANT_BUFFER_VIEW_DESC passCbvDesc = {};

        passCbvDesc.BufferLocation  = frameResource->pPassCB->GetResource()->GetGPUVirtualAddress();
        passCbvDesc.SizeInBytes     = passCBByteSize;

        pDevice->CreateConstantBufferView(&passCbvDesc, cbvHandle);
        cbvHandle.ptr += descriptorSize;

        D3D12_SHADER_RESOURCE_VIEW_DESC lightSrvDesc = {};
        lightSrvDesc.Format                     = DXGI_FORMAT_UNKNOWN; // Structured buffer has no format
        lightSrvDesc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
        lightSrvDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        lightSrvDesc.Buffer.FirstElement        = 0;
        lightSrvDesc.Buffer.NumElements         = m_maxNumberOfLights; 
        lightSrvDesc.Buffer.StructureByteStride = sizeof(sLightConstants);
        lightSrvDesc.Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;

        pDevice->CreateShaderResourceView(
            frameResource->pLightBuffer->GetResource(),
            &lightSrvDesc,
            cbvHandle);

        cbvHandle.ptr += descriptorSize;
    }


}

// --------------------------------------------------------------------------------------------------------------------------
void cDirectX12::UploadTexturesToGPU(std::vector<cTexture>& textures)
{
    ID3D12Device* pDevice = m_pDeviceManager->GetDevice();
    ID3D12DescriptorHeap* pHeap = m_pBufferManager->GetCbvHeap();
    UINT descriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    UINT baseOffset = m_pBufferManager->GetTextureOffset();
    UINT numTextures = min((UINT)textures.size(), 7u);

    // --- Temporärer CommandAllocator + CommandList ---
    ComPtr<ID3D12CommandAllocator> tempAlloc;
    ComPtr<ID3D12GraphicsCommandList> tempCmdList;

    cDirectX12Util::ThrowIfFailed(
        pDevice->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&tempAlloc)
        )
    );

    cDirectX12Util::ThrowIfFailed(
        pDevice->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            tempAlloc.Get(),
            nullptr,
            IID_PPV_ARGS(&tempCmdList)
        )
    );

    tempCmdList->Reset(tempAlloc.Get(), nullptr);

    std::wcout << L"[TEXTURE UPLOAD] " << numTextures << L" textures\n";

    // --- Upload jede Textur ---
    for (UINT i = 0; i < numTextures; ++i)
    {
        textures[i].UploadToGpu(pDevice, tempCmdList.Get());
    }

    // --- SRVs erstellen (keine Barriers hier) ---
    for (UINT i = 0; i < numTextures; ++i)
    {
        UINT heapIndex = baseOffset + i;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = pHeap->GetCPUDescriptorHandleForHeapStart();
        cpuHandle.ptr += (UINT64)heapIndex * (UINT64)descriptorSize;

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = textures[i].GetResource()->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = textures[i].GetMipLevels();

        pDevice->CreateShaderResourceView(textures[i].GetResource(), &srvDesc, cpuHandle);
        std::wcout << L"[SRV] Texture " << i << L" -> Heap " << heapIndex << L"\n";
    }

    // --- CommandList ausführen ---
    tempCmdList->Close();
    ID3D12CommandList* cmdLists[] = { tempCmdList.Get() };
    m_pDeviceManager->GetCommandQueue()->ExecuteCommandLists(1, cmdLists);

    ComPtr<ID3D12Fence> uploadFence;
    UINT64 uploadFenceValue = 1;

    pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&uploadFence));
    m_pDeviceManager->GetCommandQueue()->Signal(uploadFence.Get(), uploadFenceValue);

    if (uploadFence->GetCompletedValue() < uploadFenceValue)
    {
        HANDLE eventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        uploadFence->SetEventOnCompletion(uploadFenceValue, eventHandle);
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }


    std::wcout << L"[UPLOAD COMPLETE] SRVs ready - transition in main Draw loop\n";
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
