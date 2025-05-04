#include "directx12.h"

#include <iostream>
#include <string>
#include <comdef.h>
#include <Windows.h>
#include <d3dx12.h>

#include "directx12Util.h"
#include "window.h"
#include "timer.h"
#include "vertex.h"

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
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::InitializeVerticies(sVertex _verticies[], int _numberOfVertecies)
{
    const UINT64 vbByteSize = _numberOfVertecies * sizeof(sVertex);

    ComPtr<ID3D12Resource> pVertexBufferGPU = nullptr;
    ComPtr<ID3D12Resource> pVertexBufferUploader = nullptr;

    pVertexBufferGPU = cDirectX12Util::CreateDefaultBuffer(m_pDevice.Get(), m_pCommandList.Get(), _verticies, vbByteSize, pVertexBufferUploader);

    D3D12_VERTEX_BUFFER_VIEW vbv;

    vbv.BufferLocation = pVertexBufferGPU->GetGPUVirtualAddress();
    vbv.StrideInBytes = sizeof(sVertex);
    vbv.SizeInBytes = _numberOfVertecies * sizeof(sVertex);

    D3D12_VERTEX_BUFFER_VIEW vertexBuffers[1] = { vbv };
    m_pCommandList->IASetVertexBuffers(0, 1, vertexBuffers);

    m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_pCommandList->DrawInstanced(_numberOfVertecies, 1, 0, 0);

}

// --------------------------------------------------------------------------------------------------------------------------
// clears backbuffer, depthstencil and presents the frame to the screen

void cDirectX12::Draw()
{
    // Reset the command allocator and command list for reuse.
    ThrowIfFailed(m_pDirectCmdListAlloc->Reset());
    ThrowIfFailed(m_pCommandList->Reset(
        m_pDirectCmdListAlloc.Get(),
        nullptr // No initial PSO (Pipeline State Object) for now.
    ));

    // Transition the back buffer from PRESENT to RENDER_TARGET state.
    m_pCommandList->ResourceBarrier(
        1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            GetCurrentBackBuffer().Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        ));

    // Set the viewport for rendering.
    m_pCommandList->RSSetViewports(1, &m_viewPort);

    // Define clear color (red).
    float colorRGBA[4] = { 1.f, 0.f, 0.f, 1.f };

    // Clear the current render target with the specified color.
    m_pCommandList->ClearRenderTargetView(
        GetCurrentBackbufferView(),
        colorRGBA,
        0, nullptr
    );

    // Clear the depth/stencil buffer to default values.
    m_pCommandList->ClearDepthStencilView(
        GetDepthStencilView(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        1.f, 0, 0, nullptr
    );

    // Bind the render target and depth/stencil buffer for output-merging stage.
    m_pCommandList->OMSetRenderTargets(
        1,
        &GetCurrentBackbufferView(),
        true,
        &GetDepthStencilView()
    );

    // Transition the back buffer back to PRESENT state for display.
    m_pCommandList->ResourceBarrier(
        1, &CD3DX12_RESOURCE_BARRIER::Transition(
            GetCurrentBackBuffer().Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        ));

    // Close the command list to prepare for execution.
    ThrowIfFailed(m_pCommandList->Close());

    // Execute the recorded command list.
    ID3D12CommandList* cmdLists[] = { m_pCommandList.Get() };
    m_pCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

    // Present the rendered frame to the screen.
    ThrowIfFailed(m_pSwapChain->Present(1, 0));

    // Move to the next back buffer in the swap chain.
    m_currentBackBuffer = (m_currentBackBuffer + 1) % c_swapChainBufferCount;

    // Ensure all GPU commands are finished before continuing.
    FlushCommandQueue();
}

// --------------------------------------------------------------------------------------------------------------------------
// calculates the aspectratio of the window 

float cDirectX12::GetAspectRatio() const
{
    return m_pWindow->GetWidth()  / m_pWindow->GetHeight();
}

// --------------------------------------------------------------------------------------------------------------------------
// creates a dxgi factory and a d3d12device
// in case no gpu is found fallback to warp device

void cDirectX12::InitializeDeviceAndFactory()
{
    // Create DXGI Factory
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_pDxgiFactory)), "factory");

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

    msQualityLevels.Format              = m_backBufferFormat; 
    msQualityLevels.SampleCount         = 4;
    msQualityLevels.Flags               = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels    = 0;
    
    ThrowIfFailed(m_pDevice->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels)
    ));

    m_4xMsaaQuality = msQualityLevels.NumQualityLevels;
    std::cout << "4xMsaa quality level: " << m_4xMsaaQuality << "\n";
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

    m_pCommandList->Close();
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
    swapChainDesc.SampleDesc.Count  = 1;                                        // No anti-aliasing (MSAA disabled).
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
        D3D12_RESOURCE_STATE_COMMON,                            // Initial resource state
        &optClear,                                              // Clear value for depth and stencil
        IID_PPV_ARGS(m_pDepthStencilBuffer.GetAddressOf())      // Output resource pointer
    ));

    // Create the depth-stencil view for the resource.
    m_pDevice->CreateDepthStencilView(
        m_pDepthStencilBuffer.Get(),                         // Depth-stencil resource
        nullptr,                                             // No specific view description (defaults)
        GetDepthStencilView()                                // Call a function to get the view descriptor
    );

    // Set up the resource barrier to transition the resource state.
    D3D12_RESOURCE_BARRIER barrier = {};

    barrier.Type                    = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;   // Resource transition barrier type
    barrier.Flags                   = D3D12_RESOURCE_BARRIER_FLAG_NONE;         // No additional flags
    barrier.Transition.pResource    = m_pDepthStencilBuffer.Get();              // Resource to transition
    barrier.Transition.StateBefore  = D3D12_RESOURCE_STATE_COMMON;              // Resource state before transition
    barrier.Transition.StateAfter   = D3D12_RESOURCE_STATE_DEPTH_WRITE;         // State after transition
    barrier.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;  // Apply to all subresources

    // Execute the resource barrier command to transition the state.
    m_pCommandList->ResourceBarrier(1, &barrier);  // Apply the barrier on the command list
}

// --------------------------------------------------------------------------------------------------------------------------
// creates the viewport

void cDirectX12::InitializeViewPort()
{
    D3D12_VIEWPORT m_viewPort;

    m_viewPort.TopLeftX = 0.f;
    m_viewPort.TopLeftY = 0.f;
    m_viewPort.Width    = static_cast<float>(m_pWindow->GetWidth());
    m_viewPort.Height   = static_cast<float>(m_pWindow->GetHeight());
    m_viewPort.MinDepth = 0.f;
    m_viewPort.MaxDepth = 1.f;

    m_pCommandList->RSSetViewports(1, &m_viewPort);
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
    m_pSwapChain->ResizeBuffers(
        c_swapChainBufferCount,
        m_pWindow->GetWidth(),
        m_pWindow->GetHeight(),
        m_backBufferFormat,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

    std::cout << m_pWindow->GetWidth() << ", " << m_pWindow->GetHeight() << "\n";

    InitializeRenderTargetView();

    m_pDepthStencilBuffer->Release();
    InitializeDepthStencilView();
}

// --------------------------------------------------------------------------------------------------------------------------
