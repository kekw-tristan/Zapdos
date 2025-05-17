#include "swapChainManager.h"

#include <iostream>

#include "../Core/window.h"

#include "d3dx12.h"
#include "directx12Util.h"
#include "deviceManager.h"

// --------------------------------------------------------------------------------------------------------------------------
// Constructor

cSwapChainManager::cSwapChainManager(cDeviceManager* _pDeviceManager, cWindow* _pWindow)
	: m_pDeviceManager      (_pDeviceManager)
    , m_pWindow             (_pWindow)
	, m_pSwapChain          (nullptr)
    , m_pRtvHeap            (nullptr)
    , m_pDsvHeap            (nullptr)
    , m_viewPort            ()
    , m_pSwapChainBuffer    ()
    , m_pDepthStencilBuffer (nullptr)
{

}

// --------------------------------------------------------------------------------------------------------------------------

void cSwapChainManager::Initialize()
{
    InitializeSwapChain();
    InitializeDescriptorHeaps();
    //InitializeRenderTargetView();
    InitializeDepthStencilView();
    InitializeViewPort();
}

// --------------------------------------------------------------------------------------------------------------------------

void cSwapChainManager::OnResize()
{
    // release render targets and depth stencil
    for (int index = 0; index < c_swapChainBufferCount; ++index)
    {
        m_pSwapChainBuffer[index].Reset();
    }

    m_pDepthStencilBuffer.Reset();

    // get spawchain desc
    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    cDirectX12Util::ThrowIfFailed(m_pSwapChain->GetDesc1(&scDesc));

   
    // resize swapchain buffers
    cDirectX12Util::ThrowIfFailed(m_pSwapChain->ResizeBuffers(
        c_swapChainBufferCount,
        m_pWindow->GetWidth(),
        m_pWindow->GetHeight(),
        scDesc.Format,  
        0
    ));

    // recreate render target view
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (int index = 0; index < c_swapChainBufferCount; index++)
    {
        cDirectX12Util::ThrowIfFailed(m_pSwapChain->GetBuffer(index, IID_PPV_ARGS(&m_pSwapChainBuffer[index])));
        m_pDeviceManager->GetDevice()->CreateRenderTargetView(m_pSwapChainBuffer[index].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, m_pDeviceManager->GetDescriptorSizes().rtv);
    }

    InitializeDepthStencilView();
    InitializeViewPort();
}

// --------------------------------------------------------------------------------------------------------------------------

IDXGISwapChain4* cSwapChainManager::GetSwapChain() const
{
	return m_pSwapChain.Get();
}

// --------------------------------------------------------------------------------------------------------------------------

ID3D12DescriptorHeap* cSwapChainManager::GetRtvHeap() const
{
    return m_pRtvHeap.Get();
}

// --------------------------------------------------------------------------------------------------------------------------

ID3D12DescriptorHeap* cSwapChainManager::GetDsvHeap() const
{
    return m_pDsvHeap.Get();
}

// --------------------------------------------------------------------------------------------------------------------------

D3D12_CPU_DESCRIPTOR_HANDLE cSwapChainManager::GetCurrentBackBufferView() const
{
    UINT currentIndex = m_pSwapChain->GetCurrentBackBufferIndex();

    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(currentIndex) * static_cast<SIZE_T>(m_pDeviceManager->GetDescriptorSizes().rtv);

    return handle;
}

// --------------------------------------------------------------------------------------------------------------------------

D3D12_CPU_DESCRIPTOR_HANDLE cSwapChainManager::GetDepthStencilView() const
{
    return m_pDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

// --------------------------------------------------------------------------------------------------------------------------

ID3D12Resource* cSwapChainManager::GetCurrentBackBuffer() const
{
    return m_pSwapChainBuffer[m_pSwapChain->GetCurrentBackBufferIndex()].Get();
}

// --------------------------------------------------------------------------------------------------------------------------

D3D12_VIEWPORT& cSwapChainManager::GetViewport() 
{
    return m_viewPort;
}

// --------------------------------------------------------------------------------------------------------------------------

void cSwapChainManager::InitializeSwapChain()
{
    // Setup a DXGI_SWAP_CHAIN_DESC1 structure with swap chain properties.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};

    swapChainDesc.BufferCount           = c_swapChainBufferCount;                                                                   // Total number of buffers in the swap chain.
    swapChainDesc.Width                 = m_pWindow->GetWidth();                                                                    // Match the width of the client window.
    swapChainDesc.Height                = m_pWindow->GetHeight();                                                                   // Match the height of the client window.
    swapChainDesc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;                                                               // Use 32-bit RGBA format.
    swapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;                                                          // Buffers are used as render targets.
    swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;                                                            // Recommended swap effect for modern hardware.
    swapChainDesc.SampleDesc.Count      = m_pDeviceManager->Get4xMSAAQuality() ? 4 : 1;                                             // Enable 4x MSAA if supported, otherwise no MSAA.
    swapChainDesc.SampleDesc.Quality    = m_pDeviceManager->Get4xMSAAQuality() ? (m_pDeviceManager->Get4xMSAAQuality() - 1) : 0;    // Use highest supported MSAA quality.
    swapChainDesc.AlphaMode             = DXGI_ALPHA_MODE_UNSPECIFIED;                                                              // No specific alpha mode (not used in standard swapchains).
    swapChainDesc.Flags                 = 0;                                                                                        // Allow fullscreen toggle with Alt+Enter.

    ComPtr<IDXGISwapChain1> swapChain;

    // Create a swap chain tied to the application's window.
    cDirectX12Util::ThrowIfFailed(m_pDeviceManager->GetDxgiFactory()->CreateSwapChainForHwnd(
        m_pDeviceManager->GetCommandQueue(),    // Command queue used for presentation.
        m_pWindow->GetHWND(),                   // Handle to the output window.
        &swapChainDesc,                         // Description of the swap chain.
        nullptr,                                // No fullscreen descriptor.
        nullptr,                                // No output restriction.
        &swapChain                              // Output parameter for the created swap chain.
    ));

    cDirectX12Util::ThrowIfFailed(m_pDeviceManager->GetDxgiFactory()->MakeWindowAssociation(m_pWindow->GetHWND(), DXGI_MWA_NO_ALT_ENTER));

    // Convert the created swap chain to IDXGISwapChain4 for access to newer DXGI features.
    cDirectX12Util::ThrowIfFailed(swapChain.As(&m_pSwapChain));


}

// --------------------------------------------------------------------------------------------------------------------------

void cSwapChainManager::InitializeDescriptorHeaps()
{
    // Describe the RTV (Render Target View) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC rtvHD;
    rtvHD.NumDescriptors = c_swapChainBufferCount;  // One descriptor per swap chain buffer.
    rtvHD.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;    // Heap type is RTV.
    rtvHD.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;  // No shader access needed for RTV.
    rtvHD.NodeMask = 0;                             // For single GPU operation.

    // Create the RTV descriptor heap.
    cDirectX12Util::ThrowIfFailed(m_pDeviceManager->GetDevice()->CreateDescriptorHeap(
        &rtvHD,
        IID_PPV_ARGS(m_pRtvHeap.GetAddressOf())
    ));

    // Now we need to create the RTVs (Render Target Views) in the heap.
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRtvHeap->GetCPUDescriptorHandleForHeapStart());
    UINT rtvDescriptorSize = m_pDeviceManager->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create RTVs for each back buffer in the swap chain.
    for (UINT i = 0; i < c_swapChainBufferCount; ++i)
    {
        // Get the swap chain buffer (back buffer)
        cDirectX12Util::ThrowIfFailed(m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pSwapChainBuffer[i])));

        // Create the render target view for the back buffer
        m_pDeviceManager->GetDevice()->CreateRenderTargetView(
            m_pSwapChainBuffer[i].Get(), // The swap chain buffer resource
            nullptr,                     // Default format
            rtvHandle                    // The handle in the RTV heap where to place the descriptor
        );

        // Move to the next descriptor in the heap
        rtvHandle.Offset(1, rtvDescriptorSize);
    }

    // Describe the DSV (Depth Stencil View) descriptor heap.
    D3D12_DESCRIPTOR_HEAP_DESC dsvHD;
    dsvHD.NumDescriptors = 1;                       // Only one DSV needed.
    dsvHD.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;    // Heap type is DSV.
    dsvHD.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;  // No shader access needed for DSV.
    dsvHD.NodeMask = 0;                             // For single GPU operation.

    // Create the DSV descriptor heap.
    cDirectX12Util::ThrowIfFailed(m_pDeviceManager->GetDevice()->CreateDescriptorHeap(
        &dsvHD,
        IID_PPV_ARGS(m_pDsvHeap.GetAddressOf())
    ));
}


// --------------------------------------------------------------------------------------------------------------------------

void cSwapChainManager::InitializeDepthStencilView()
{
    // Set up the resource description for the depth-stencil buffer.
    D3D12_RESOURCE_DESC dsDesc = {};

    dsDesc.Dimension            = D3D12_RESOURCE_DIMENSION_TEXTURE2D;                                                       // The resource is a 2D texture.
    dsDesc.Alignment            = 0;                                                                                        // Use default alignment.
    dsDesc.Width                = m_pWindow->GetWidth();                                                                    // Width matches the screen width.
    dsDesc.Height               = m_pWindow->GetHeight();                                                                   // Height matches the screen height.
    dsDesc.DepthOrArraySize     = 1;                                                                                        // A single depth slice (not a texture array).
    dsDesc.MipLevels            = 1;                                                                                        // No mip levels — just a single texture level.
    dsDesc.Format               = c_depthStencilFormat;                                                                     // Format of the depth-stencil buffer (e.g., DXGI_FORMAT_D24_UNORM_S8_UINT).
    dsDesc.SampleDesc.Count     = m_pDeviceManager->Get4xMSAAQuality() ? 4 : 1;                                             // Use 4x MSAA if supported, otherwise 1 sample (no MSAA).
    dsDesc.SampleDesc.Quality   = m_pDeviceManager->Get4xMSAAQuality() ? (m_pDeviceManager->Get4xMSAAQuality() - 1) : 0;    // Use highest supported MSAA quality index.
    dsDesc.Layout               = D3D12_TEXTURE_LAYOUT_UNKNOWN;                                                             // Let the driver choose the layout.
    dsDesc.Flags                = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;                                                  // Resource will be used as a depth-stencil target.

    // Set up the clear value for depth and stencil.
    D3D12_CLEAR_VALUE optClear = {};

    optClear.Format                 = c_depthStencilFormat;     // Format must match the resource format.
    optClear.DepthStencil.Depth     = 1.0f;                     // Depth value to clear to (1.0f = far plane).
    optClear.DepthStencil.Stencil   = 0;                        // Stencil value to clear to.

    // Set up the heap properties for the resource (Default heap).
    D3D12_HEAP_PROPERTIES heapProps = {};

    heapProps.Type                  = D3D12_HEAP_TYPE_DEFAULT;          // GPU-only memory.
    heapProps.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;  // Use default.
    heapProps.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;        // Use default pool.
    heapProps.CreationNodeMask      = 1;                                // Only using a single GPU (node 0).
    heapProps.VisibleNodeMask       = 1;                                // Resource visible to that GPU.;

    // Create the committed depth-stencil resource.
    cDirectX12Util::ThrowIfFailed(m_pDeviceManager->GetDevice()->CreateCommittedResource(
        &heapProps,                                             // Heap properties
        D3D12_HEAP_FLAG_NONE,                                   // No heap flags
        &dsDesc,                                                // Resource description
        D3D12_RESOURCE_STATE_DEPTH_WRITE,                       // Initial resource state
        &optClear,                                              // Clear value for depth and stencil
        IID_PPV_ARGS(m_pDepthStencilBuffer.GetAddressOf())      // Output resource pointer
    ));

    // Create the committed depth-stencil resource on the GPU.
    cDirectX12Util::ThrowIfFailed(m_pDeviceManager->GetDevice()->CreateCommittedResource(
        &heapProps,                                         // Memory heap properties.
        D3D12_HEAP_FLAG_NONE,                               // No special flags.
        &dsDesc,                                            // Resource description (texture, size, format, MSAA, etc.).
        D3D12_RESOURCE_STATE_DEPTH_WRITE,                   // Initial state: ready for depth writing.
        &optClear,                                          // Clear values (for depth and stencil).
        IID_PPV_ARGS(m_pDepthStencilBuffer.GetAddressOf())  // Output pointer for the created resource.
    ));

    // Create the depth-stencil view for the resource.
    m_pDeviceManager->GetDevice()->CreateDepthStencilView(
        m_pDepthStencilBuffer.Get(),                         // Depth-stencil resource
        nullptr,                                             // No specific view description (defaults)
        GetDepthStencilView()                                // Call a function to get the view descriptor
    );

    m_pDepthStencilBuffer->SetName(L"DepthStencilBuffer");
 }

// --------------------------------------------------------------------------------------------------------------------------

void cSwapChainManager::InitializeViewPort()
{
    m_viewPort.TopLeftX = 0.f;
    m_viewPort.TopLeftY = 0.f;
    m_viewPort.Width = static_cast<float>(m_pWindow->GetWidth());
    m_viewPort.Height = static_cast<float>(m_pWindow->GetHeight());
    m_viewPort.MinDepth = 0.f;
    m_viewPort.MaxDepth = 1.f;

    m_pDeviceManager->GetCommandList()->RSSetViewports(1, &m_viewPort);
}

// --------------------------------------------------------------------------------------------------------------------------
