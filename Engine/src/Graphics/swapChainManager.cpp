#include "swapChainManager.h"

#include "../Core/window.h"

#include "directx12Util.h"
#include "deviceManager.h"

// --------------------------------------------------------------------------------------------------------------------------
// Constructor

cSwapChainManager::cSwapChainManager(cDeviceManager* _pDeviceManager, cWindow* _pWindow)
	: m_pDeviceManager(_pDeviceManager)
    , m_pWindow(_pWindow)
	, m_pSwapChain(nullptr)
{

}

// --------------------------------------------------------------------------------------------------------------------------

void cSwapChainManager::Initialize()
{
}

// --------------------------------------------------------------------------------------------------------------------------

IDXGISwapChain4* cSwapChainManager::GetSwapChain()
{
	return m_pSwapChain.Get();
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
    swapChainDesc.Flags                 = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;                                                   // Allow fullscreen toggle with Alt+Enter.

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

void cSwapChainManager::InitializeRenderTargetView()
{
    // Get the starting CPU descriptor handle for the RTV (Render Target View) heap.
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pRtvHeap->GetCPUDescriptorHandleForHeapStart();

    // Loop over each buffer in the swap chain.
    for (int index = 0; index < c_swapChainBufferCount; index++)
    {
        // Retrieve the swap chain back buffer at the specified index.
        cDirectX12Util::ThrowIfFailed(m_pSwapChain->GetBuffer(
            index,
            IID_PPV_ARGS(&m_pSwapChainBuffer[index])  // Store in swap chain buffer array.
        ));

        // Create a render target view (RTV) for the back buffer.
        m_pDeviceManager->GetDevice()->CreateRenderTargetView(
            m_pSwapChainBuffer[index].Get(),  // The back buffer resource.
            nullptr,                          // Use default RTV description.
            rtvHandle                         // Descriptor handle to write the RTV into.
        );

        // Move to the next descriptor in the RTV heap.
        rtvHandle.ptr += m_pDeviceManager->GetDescriptorSizes()->rtv;

        m_pSwapChainBuffer[index]->SetName(L"Backbuffer");
    }
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

    m_pDepthStencilBuffer->SetName(L"DepthStencilBuffer");
 }

// --------------------------------------------------------------------------------------------------------------------------
