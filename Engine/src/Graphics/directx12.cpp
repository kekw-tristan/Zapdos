#include "directx12.h"

#include <array>
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

#include "directx12Util.h"

#include "uploadBuffer.h"
#include "vertex.h"
#include "meshGeometry.h"

constexpr float c_pi = 3.1415927f;

// possible things to add:
// sissor rectangles(pixels outside the rectangle area are culled [useful for gui optimization])

// --------------------------------------------------------------------------------------------------------------------------
// throws exception when HardwareResult fails

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
            
            std::cout << "DirectX call failed. HRESULT = " << std::to_string(hr) << "\n";
           
            throw std::runtime_error("DirectX call failed. HRESULT = " + std::to_string(hr));
    }
}

inline void ThrowIfFailed(HRESULT hr, std::string _message)
{
    if (FAILED(hr))
    {
        _com_error err(hr);
        std::wcout << L"Error: " << err.ErrorMessage() << std::endl; 

        std::cout << "DirectX call failed. HRESULT = " << std::to_string(hr) << _message << "\n";
        throw std::runtime_error("DirectX call failed. HRESULT = " + std::to_string(hr));
    }
}


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
    
    if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
    {
        LoadLibrary(GetLatestWinPixGpuCapturerPath_Cpp17().c_str());
    }

    m_backBufferFormat      = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_depthStencilFormat    = DXGI_FORMAT_D24_UNORM_S8_UINT;
    
    m_pWindow = _pWindow; 
    m_pTimer = _pTimer;

    std::cout << "Initialize DirectX12\n";
    
    std::cout << "Initialize device and factory\n";
    InitializeDeviceAndFactory();
    
    std::cout << "Initialize fence and descriptor\n";
    InitializeFenceAndDescriptor();
    
    std::cout << "Check 4xMSAA quality support\n";
    Check4XMSAAQualitySupport();

    std::cout << "Initialize command queue and list\n";
    InitializeCommandQueueAndList();

    std::cout << "Initialize swap chain\n";
    InitializeSwapChain();

    std::cout << "Initialize escriptor heaps\n";
    InitializeDescriptorHeaps();


    std::cout << "Initialize render target view\n";
    InitializeRenderTargetView();

    std::cout << "Initialize depth stencil view\n";
    InitializeDepthStencilView();

    std::cout << "Initialize viewport\n";
    InitializeViewPort();
    
    std::cout << "Initialize Root Signature\n";
    InitializeRootSignature();
    InitializeConstantBuffer();

    std::cout << "Initialize shader\n";
    InitializeShader();

    std::cout << "Initialize vertices\n";
    InitializeVertices();

    std::cout << "Initialize pipeline state object\n";
    InitializePipelineStateObject();

    m_pCommandList->Close();
    ID3D12CommandList* cmdLists[] = { m_pCommandList.Get() };
    m_pCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
    FlushCommandQueue();
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::Finalize()
{
    delete m_pObjectCB;
    delete m_pBoxGeometry;
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
    ThrowIfFailed(D3DCreateBlob(vbByteSize, &m_pBoxGeometry->vertexBufferCPU));
    CopyMemory(m_pBoxGeometry->vertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &m_pBoxGeometry->indexBufferCPU));
    CopyMemory(m_pBoxGeometry->indexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    // Create and name GPU buffers
    m_pBoxGeometry->vertexBufferGPU = cDirectX12Util::CreateDefaultBuffer(
        m_pDevice.Get(),
        m_pCommandList.Get(),
        vertices.data(),
        vbByteSize,
        m_pBoxGeometry->vertexBufferUploader
    );
    m_pBoxGeometry->vertexBufferGPU->SetName(L"Box_VertexBuffer_GPU");

    m_pBoxGeometry->indexBufferGPU = cDirectX12Util::CreateDefaultBuffer(
        m_pDevice.Get(),
        m_pCommandList.Get(),
        indices.data(),
        ibByteSize,
        m_pBoxGeometry->indexBufferUploader
    );
    m_pBoxGeometry->indexBufferGPU->SetName(L"Box_IndexBuffer_GPU");

    // Ensure the upload buffers are initialized before setting names
    if (m_pBoxGeometry->vertexBufferUploader != nullptr) {
        m_pBoxGeometry->vertexBufferUploader->SetName(L"Box_VertexBuffer_Uploader");
    }
    else {
        std::cerr << "Error: vertexBufferUploader is nullptr." << std::endl;
    }

    if (m_pBoxGeometry->indexBufferUploader != nullptr) {
        m_pBoxGeometry->indexBufferUploader->SetName(L"Box_IndexBuffer_Uploader");
    }
    else {
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

void cDirectX12::InitializeShader()
{
    m_pVsByteCode = cDirectX12Util::CompileShader(L"..\\Assets\\Shader\\shader.hlsl", nullptr, "VS", "vs_5_0");
    m_pPsByteCode = cDirectX12Util::CompileShader(L"..\\Assets\\Shader\\shader.hlsl", nullptr, "PS", "ps_5_0");
    
    m_InputLayouts =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    
    std::cout << "vertexshader size: " << m_pVsByteCode->GetBufferSize() << std::endl;

}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::InitializeConstantBuffer()
{
    // Step 1: Initialize the object constant buffer (already done, assuming cUploadBuffer is a template that creates and manages the buffer)
    m_pObjectCB = new cUploadBuffer<sObjectConstants>(m_pDevice.Get(), 1, true);  // Assuming 1 object in the buffer

    // Step 2: Calculate the buffer size for an object constant
    UINT objCBByteSize = cDirectX12Util::CalculateBufferByteSize(sizeof(sObjectConstants));  // Assuming sizeof(sObjectConstants) is the correct size

    // Step 3: Get the GPU virtual address of the constant buffer
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_pObjectCB->GetResource()->GetGPUVirtualAddress();

    // Step 4: Calculate the offset for the ith object in the buffer (i = 0 in this case)
    int boxCBufIndex = 0;
    cbAddress += boxCBufIndex * objCBByteSize;  // Get the address for the specific constant buffer slot (i = 0 in this case)

    // Step 5: Set up the constant buffer view (CBV)
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = cbAddress;  // GPU virtual address of the constant buffer
    cbvDesc.SizeInBytes = cDirectX12Util::CalculateBufferByteSize(sizeof(sObjectConstants));  // The buffer size, aligned to 256 bytes

    // Step 6: Create the constant buffer view using the descriptor
    m_pDevice->CreateConstantBufferView(&cbvDesc, m_pCbvHeap->GetCPUDescriptorHandleForHeapStart());

}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::Update(XMMATRIX view)
{
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
    XMStoreFloat4x4(&objConstants.worldViewProj, worldViewProjT);

    // Upload to mapped constant buffer (frame index 0 for now)
    m_pObjectCB->CopyData(0, objConstants);

}

// --------------------------------------------------------------------------------------------------------------------------
// clears backbuffer, depthstencil and presents the frame to the screen

void cDirectX12::Draw()
{
    // === Reset allocator & command list BEFORE recording ===
    ThrowIfFailed(m_pDirectCmdListAlloc->Reset());
    ThrowIfFailed(m_pCommandList->Reset(m_pDirectCmdListAlloc.Get(), m_pPso.Get())); // Bind PSO

    // === Set viewport and scissor ===
    m_pCommandList->RSSetViewports(1, &m_viewPort);

    D3D12_RECT m_scissorRect = { 0, 0, m_pWindow->GetWidth(), m_pWindow->GetHeight() };
    m_pCommandList->RSSetScissorRects(1, &m_scissorRect);

    // === Transition back buffer from PRESENT to RENDER_TARGET ===
    m_pCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
            GetCurrentBackBuffer().Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        )
    );

    // === Clear render target ===
    float clearColor[] = { 0.f, 0.f, 0.f, 1.f }; // Magenta
    m_pCommandList->ClearRenderTargetView(GetCurrentBackbufferView(), clearColor, 0, nullptr);

    

    // === Clear depth/stencil buffer ===
    m_pCommandList->ClearDepthStencilView(
        GetDepthStencilView(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.0f, 0,
        0, nullptr
    );

    // === Set render targets ===
    m_pCommandList->OMSetRenderTargets(1, &GetCurrentBackbufferView(), TRUE, &GetDepthStencilView());

    // === Bind descriptor heap ===
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_pCbvHeap.Get() };
    m_pCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // === Set root signature ===
    m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());

    // === Input assembler: vertex/index buffers ===
    m_pCommandList->IASetVertexBuffers(0, 1, &m_pBoxGeometry->GetVertexBufferView());
    m_pCommandList->IASetIndexBuffer(&m_pBoxGeometry->GetIndexBufferView());
    m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    std::cout << m_pBoxGeometry->drawArguments["box"].indexCount << std::endl;

    // === Set constant buffer view (CBV) ===
    m_pCommandList->SetGraphicsRootDescriptorTable(0, m_pCbvHeap->GetGPUDescriptorHandleForHeapStart());

    // === Draw geometry ===
    m_pCommandList->DrawIndexedInstanced(
        m_pBoxGeometry->drawArguments["box"].indexCount,
        1, 0, 0, 0
    );

    // === Transition back buffer from RENDER_TARGET to PRESENT ===
    m_pCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
            GetCurrentBackBuffer().Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        )
    );

    // === Close and execute command list ===
    ThrowIfFailed(m_pCommandList->Close());

    ID3D12CommandList* cmdLists[] = { m_pCommandList.Get() };
    m_pCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // === Present ===
    ThrowIfFailed(m_pSwapChain->Present(1, 0));

    // === Move to next back buffer ===
    m_currentBackBuffer = (m_currentBackBuffer + 1) % c_swapChainBufferCount;

    FlushCommandQueue();

}


// --------------------------------------------------------------------------------------------------------------------------
// calculates the aspectratio of the window 

float cDirectX12::GetAspectRatio() const
{
    return static_cast<float>(m_pWindow->GetWidth())  / static_cast<float>(m_pWindow->GetHeight());
}

// --------------------------------------------------------------------------------------------------------------------------
// creates a dxgi factory and a d3d12device
// in case no gpu is found fallback to warp device

void cDirectX12::InitializeDeviceAndFactory()
{
    // Create DXGI Factory
    ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&m_pDxgiFactory)), "factory");

    // Create Device
    HRESULT hr = D3D12CreateDevice(
        nullptr,                  // default adapter
        D3D_FEATURE_LEVEL_11_0,   // feature level
        IID_PPV_ARGS(&m_pDevice)  // device output
    );
    // in case no gpu is found rendering on cpu emulated
    // WARP = Windows Advanced Rasterization Platform
    if (FAILED(hr))
    {
        std::cout << "emulating on cpu\n";
        ComPtr<IDXGIAdapter> pWarpAdapter;

        ThrowIfFailed(m_pDxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

        
        ThrowIfFailed(D3D12CreateDevice(
            pWarpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_pDevice)
        ));
    }
}

// --------------------------------------------------------------------------------------------------------------------------
// creates fence and gets descriptor sizes
// CBV/SRV/UAV describes constant buffers, shader recources and unordered access view recources
// RTV describe render target resources
// DSV describe depth/stencil resources 

void cDirectX12::InitializeFenceAndDescriptor()
{
    // creating fence
    ThrowIfFailed(m_pDevice->CreateFence(
        0,                          // initial value
        D3D12_FENCE_FLAG_NONE,      // flags 
        IID_PPV_ARGS(&m_pFence)     // fence output
    ));

    // get descriptor sizes
    m_rtvDescriptorSize     = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);          
    m_dsvDescriptorSize     = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_cbvSrvDescriptorSize  = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); 

}

// --------------------------------------------------------------------------------------------------------------------------
// Checks the 4xmsaa quality support level

void cDirectX12::Check4XMSAAQualitySupport()
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels = {};

    msQualityLevels.Format = m_backBufferFormat;
    msQualityLevels.SampleCount = 4;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;

    HRESULT hr = m_pDevice->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels)
    );

    if (FAILED(hr))
    {
        std::cerr << "Failed to query MSAA quality support." << std::endl;
        return; // Early exit in case of failure
    }

    // Ensure the quality level is valid
    if (msQualityLevels.NumQualityLevels > 0)
    {
        m_4xMsaaQuality = msQualityLevels.NumQualityLevels - 1;  // Use the highest available quality
        std::cout << "4x MSAA supported, quality level: " << m_4xMsaaQuality << "\n";
    }
    else
    {
        m_4xMsaaQuality = 0; // No MSAA support
        std::cout << "4x MSAA not supported, quality level set to 0." << std::endl;
    }


    msQualityLevels.SampleCount = 2;  // Check for 2x MSAA support
    msQualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(m_pDevice->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels)
    ));
    std::cout << "2x MSAA supported, quality level: " << msQualityLevels.NumQualityLevels << std::endl;

    msQualityLevels.SampleCount = 8;  // Check for 8x MSAA support
    msQualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(m_pDevice->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels)
    ));
    std::cout << "8x MSAA quality level: " << msQualityLevels.NumQualityLevels << std::endl;
}

// --------------------------------------------------------------------------------------------------------------------------
// creates command queue and command list

void cDirectX12::InitializeCommandQueueAndList()
{
    // creating command queue 
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};

    queueDesc.Type  = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    
    ThrowIfFailed(m_pDevice->CreateCommandQueue(
        &queueDesc,
        IID_PPV_ARGS(&m_pCommandQueue)
    ));

    // creating direct command list allocator
    ThrowIfFailed(m_pDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(m_pDirectCmdListAlloc.GetAddressOf())
    ));

    // creating command list 
    ThrowIfFailed(m_pDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_pDirectCmdListAlloc.Get(),                    // associated command allocator
        nullptr,                                        // initial pipeline state object
        IID_PPV_ARGS(m_pCommandList.GetAddressOf())
    ));

}

// --------------------------------------------------------------------------------------------------------------------------
// discrads old swapchain and creates new one 
// Todo: needs to be a little bit more reusable for later 

void cDirectX12::InitializeSwapChain()
{
    // Setup a DXGI_SWAP_CHAIN_DESC1 structure with swap chain properties.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

    swapChainDesc.BufferCount       = c_swapChainBufferCount;                   // Total number of buffers in the swap chain.
    swapChainDesc.Width             = m_pWindow->GetWidth();                    // Match the width of the client window.
    swapChainDesc.Height            = m_pWindow->GetHeight();                   // Match the height of the client window.
    swapChainDesc.Format            = DXGI_FORMAT_R8G8B8A8_UNORM;               // Use 32-bit RGBA format.
    swapChainDesc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;          // Buffers are used as render targets.
    swapChainDesc.SwapEffect        = DXGI_SWAP_EFFECT_FLIP_DISCARD;            // Recommended swap effect for modern hardware.
    swapChainDesc.SampleDesc.Count  = m_4xMsaaQuality ? 4 : 1;
    swapChainDesc.SampleDesc.Quality = m_4xMsaaQuality ? (m_4xMsaaQuality - 1) : 0;                                      
    swapChainDesc.AlphaMode         = DXGI_ALPHA_MODE_UNSPECIFIED;              // No alpha blending control.
    swapChainDesc.Flags             = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;   // Allow alt+enter fullscreen switching.

    ComPtr<IDXGISwapChain1> swapChain;

    // Create a swap chain tied to the application's window.
    ThrowIfFailed(m_pDxgiFactory->CreateSwapChainForHwnd(
        m_pCommandQueue.Get(),             // Command queue used for presentation.
        m_pWindow->GetHWND(),              // Handle to the output window.
        &swapChainDesc,                    // Description of the swap chain.
        nullptr,                           // No fullscreen descriptor.
        nullptr,                           // No output restriction.
        &swapChain                         // Output parameter for the created swap chain.
    ));

    // Convert the created swap chain to IDXGISwapChain4 for access to newer DXGI features.
    ThrowIfFailed(swapChain.As(&m_pSwapChain));
}

// --------------------------------------------------------------------------------------------------------------------------
// creates descriptor heaps for rtv and dsv

void cDirectX12::InitializeDescriptorHeaps()
{
    // Describe the RTV (Render Target View) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHD;

    rtvHD.NumDescriptors    = c_swapChainBufferCount;           // One descriptor per swap chain buffer.
    rtvHD.Type              = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   // Heap type is RTV.
    rtvHD.Flags             = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;  // No shader access needed for RTV.
    rtvHD.NodeMask          = 0;                                // For single GPU operation.

    // Create the RTV descriptor heap.
    ThrowIfFailed(m_pDevice->CreateDescriptorHeap(
        &rtvHD,
        IID_PPV_ARGS(m_pRtvHeap.GetAddressOf())
    ));

    // Describe the DSV (Depth Stencil View) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC dsvHD;
    dsvHD.NumDescriptors    = 1;                                // Only one DSV needed.
    dsvHD.Type              = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;   // Heap type is DSV.
    dsvHD.Flags             = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;  // No shader access needed for DSV.
    dsvHD.NodeMask          = 0;                                // For single GPU operation.

    // Create the DSV descriptor heap.
    ThrowIfFailed(m_pDevice->CreateDescriptorHeap(
        &dsvHD,
        IID_PPV_ARGS(m_pDsvHeap.GetAddressOf())
    ));
}

// --------------------------------------------------------------------------------------------------------------------------
// gets the swapchain buffers and creates a render target view for them

void cDirectX12::InitializeRenderTargetView()
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
    
    for (int index = 0; index < c_swapChainBufferCount; index++)
    {
        ThrowIfFailed(m_pSwapChain->GetBuffer(
            index,
            IID_PPV_ARGS(&m_pSwapChainBuffer[index])
        ));

        m_pDevice->CreateRenderTargetView(
            m_pSwapChainBuffer[index].Get(),
            nullptr,
            rtvHandle
        );

        rtvHandle.ptr += m_rtvDescriptorSize;

        m_pSwapChainBuffer[index]->SetName(L"Backbuffer");
    }
}

// --------------------------------------------------------------------------------------------------------------------------
// this function sets up a depth stencil buffer, creates a view for it and ensures the resource is in the 
// appropiate state for depth writing during rendering

void cDirectX12::InitializeDepthStencilView()
{
    // Set up the resource description for the depth-stencil buffer.
    D3D12_RESOURCE_DESC dsDesc = {};

    dsDesc.Dimension            = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    dsDesc.Alignment            = 0;
    dsDesc.Width                = m_pWindow->GetWidth();                        // Set resource width (screen width)
    dsDesc.Height               = m_pWindow->GetHeight();                       // Set resource height (screen height)
    dsDesc.DepthOrArraySize     = 1;                                            // Single depth slice (no array of slices)
    dsDesc.MipLevels            = 1;                                            // No mipmaps, just a single level
    dsDesc.Format               = m_depthStencilFormat;                         // Format of the depth-stencil buffer
    dsDesc.SampleDesc.Count     = m_4xMsaaQuality ? 4 : 1;                      // Set MSAA samples based on quality
    dsDesc.SampleDesc.Quality   = m_4xMsaaQuality ? (m_4xMsaaQuality - 1) : 0;  // MSAA quality
    dsDesc.Layout               = D3D12_TEXTURE_LAYOUT_UNKNOWN;                 // Default layout (can be used later)
    dsDesc.Flags                = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;      // Set the resource flag to allow depth-stencil

    // Set up the clear value for depth and stencil.
    D3D12_CLEAR_VALUE optClear = {};

    optClear.Format                 = m_depthStencilFormat;     // Format must match resource format
    optClear.DepthStencil.Depth     = 1.0f;                     // Clear depth to 1.0f (far plane)
    optClear.DepthStencil.Stencil   = 0;                        // Clear stencil to 0

    // Set up the heap properties for the resource (Default heap).
    D3D12_HEAP_PROPERTIES heapProps = {};

    heapProps.Type                  = D3D12_HEAP_TYPE_DEFAULT;          // Use default heap type for GPU access
    heapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;  // Default value (no specific CPU restrictions)
    heapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;        // Default value
    heapProps.CreationNodeMask      = 1;                                // Single GPU node (default)
    heapProps.VisibleNodeMask       = 1;                                // Resource is visible to all nodes (default)

    // Create the committed depth-stencil resource.
    ThrowIfFailed(m_pDevice->CreateCommittedResource(
        &heapProps,                                             // Heap properties
        D3D12_HEAP_FLAG_NONE,                                   // No heap flags
        &dsDesc,                                                // Resource description
        D3D12_RESOURCE_STATE_DEPTH_WRITE,                            // Initial resource state
        &optClear,                                              // Clear value for depth and stencil
        IID_PPV_ARGS(m_pDepthStencilBuffer.GetAddressOf())      // Output resource pointer
    ));

    // Create the depth-stencil view for the resource.
    m_pDevice->CreateDepthStencilView(
        m_pDepthStencilBuffer.Get(),                         // Depth-stencil resource
        nullptr,                                             // No specific view description (defaults)
        GetDepthStencilView()                                // Call a function to get the view descriptor
    );

    m_pDepthStencilBuffer->SetName(L"DepthStencilBuffer");
/*
    m_pCommandList->ResourceBarrier(
        1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            m_pDepthStencilBuffer.Get(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            D3D12_RESOURCE_STATE_DEPTH_WRITE));
    // Apply the barrier on the command list
    */
}

// --------------------------------------------------------------------------------------------------------------------------
// creates the viewport

void cDirectX12::InitializeViewPort()
{
    m_viewPort.TopLeftX = 0.f;
    m_viewPort.TopLeftY = 0.f;
    m_viewPort.Width    = static_cast<float>(m_pWindow->GetWidth());
    m_viewPort.Height   = static_cast<float>(m_pWindow->GetHeight());
    m_viewPort.MinDepth = 0.f;
    m_viewPort.MaxDepth = 1.f;

    m_pCommandList->RSSetViewports(1, &m_viewPort);
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::InitializeRootSignature()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;

    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;  // Ensure this is shader visible
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.NodeMask = 0;

    ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_pCbvHeap)));

    // ------ code above in buffermanager

    // Define the root parameter (here, binding descriptor table for the CBV)
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];

    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
        1, // number of descriptors in table
        0  // base shader register (0 for this case)
    );

    slotRootParameter[0].InitAsDescriptorTable(
        1,              // number of ranges
        &cbvTable       // pointer to array of ranges
    );

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
        1, // number of root parameters
        slotRootParameter,
        0,
        nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    );

    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;

    ThrowIfFailed(D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig,
        &errorBlob
    ));

    ThrowIfFailed(m_pDevice->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&m_pRootSignature)
    ));

    // Set the root signature for the command list (this must be done before rendering)
    m_pCommandList->SetGraphicsRootSignature(m_pRootSignature.Get());

    // Setting the descriptor heap for the command list (make sure the heap is set before issuing draw calls)
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_pCbvHeap.Get() };
    m_pCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // Set the CBV descriptor table to the root parameter at slot 0
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbv(m_pCbvHeap->GetGPUDescriptorHandleForHeapStart());
    m_pCommandList->SetGraphicsRootDescriptorTable(0, cbv);

    // Note: You also need to ensure your pipeline state object (PSO) is using this root signature.
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::InitializeRasterierState()
{
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::InitializePipelineStateObject()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    psoDesc.InputLayout = { m_InputLayouts.data(), static_cast<UINT>(m_InputLayouts.size()) };
    psoDesc.pRootSignature = m_pRootSignature.Get();
    psoDesc.VS = { reinterpret_cast<BYTE*>(m_pVsByteCode->GetBufferPointer()), m_pVsByteCode->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<BYTE*>(m_pPsByteCode->GetBufferPointer()), m_pPsByteCode->GetBufferSize() };
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = m_backBufferFormat;
    psoDesc.SampleDesc.Count = m_4xMsaaQuality ? 4 : 1;
    psoDesc.SampleDesc.Quality = m_4xMsaaQuality ? (m_4xMsaaQuality - 1) : 0;
    psoDesc.DSVFormat = m_depthStencilFormat;

    ThrowIfFailed(m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPso)));
}

// --------------------------------------------------------------------------------------------------------------------------
// returns the cpu descriptor handle pointing to the rtv for the
// currently active backbuffer in the swapchain

D3D12_CPU_DESCRIPTOR_HANDLE cDirectX12::GetCurrentBackbufferView() const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(m_currentBackBuffer) * static_cast<SIZE_T>(m_rtvDescriptorSize);
    return handle;
}

// --------------------------------------------------------------------------------------------------------------------------
// returns the cpu descriptor hand pointing to the start of the dsv

D3D12_CPU_DESCRIPTOR_HANDLE cDirectX12::GetDepthStencilView() const
{
    return m_pDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

ComPtr<ID3D12Resource> cDirectX12::GetCurrentBackBuffer() const
{
    int backbufferIndex = m_pSwapChain->GetCurrentBackBufferIndex();

    ComPtr<ID3D12Resource> pBackBuffer;
    ThrowIfFailed(m_pSwapChain->GetBuffer(backbufferIndex, IID_PPV_ARGS(&pBackBuffer)));
    return pBackBuffer;
}

// --------------------------------------------------------------------------------------------------------------------------
// Flushes the command queue to ensure that all previously submitted GPU commands are completed.

void cDirectX12::FlushCommandQueue()
{
    // Increment the fence value to mark the current set of GPU commands.
    m_currentFence++;

    // Signal the fence with the current fence value. This tells the GPU to set the fence to this value when it has finished processing all commands up to this point.
    ThrowIfFailed(m_pCommandQueue->Signal(m_pFence.Get(), m_currentFence));

    // If the GPU has not yet reached the current fence value, wait for it.
    if (m_pFence->GetCompletedValue() < m_currentFence)
    {
        // Create an event handle for GPU synchronization.
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        // Instruct the fence to trigger the event when it reaches the current fence value.
        ThrowIfFailed(m_pFence->SetEventOnCompletion(m_currentFence, eventHandle));

        // Wait for the GPU to complete execution up to the current fence value.
        WaitForSingleObject(eventHandle, INFINITE);

        // Clean up the event handle.
        CloseHandle(eventHandle);
    }
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
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * c_pi ,GetAspectRatio(), 0.1f, 1000.0f);
    XMStoreFloat4x4(&m_proj, P);
}

// --------------------------------------------------------------------------------------------------------------------------

ComPtr<ID3D12Device> cDirectX12::GetDevice() const
{
    return m_pDevice;
}

// --------------------------------------------------------------------------------------------------------------------------

ComPtr<ID3D12GraphicsCommandList> cDirectX12::GetCommandList() const
{
    return m_pCommandList;
}

// --------------------------------------------------------------------------------------------------------------------------
