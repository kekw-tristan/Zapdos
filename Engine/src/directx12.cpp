#include "directx12.h"

#include <iostream>
#include <string>

#include "window.h"

// --------------------------------------------------------------------------------------------------------------------------
// throws exception when HardwareResult fails

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
            throw std::runtime_error("DirectX call failed. HRESULT = " + std::to_string(hr));
    }
}

// --------------------------------------------------------------------------------------------------------------------------
// initializes all the directx12 components

void cDirectX12::Initialize(cWindow* _pWindow)
{
    // activate debug layer
    #if defined(DEBUG) || defined(_DEBUG) 
        {
            ComPtr<ID3D12Debug> debugController;
            ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
            debugController->EnableDebugLayer();
        }
    #endif

    m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    m_pWindow = _pWindow; 

    std::cout << "Initialize DirectX12\n";

    InitializeDeviceAndFactory();
    InitializeFenceAndDescriptor();
    Check4XMSAAQualitySupport();
    InitializeCommandQueueAndList();
}

// --------------------------------------------------------------------------------------------------------------------------
// creates a dxgi factory and a d3d12device
// in case no gpu is found fallback to warp device

void cDirectX12::InitializeDeviceAndFactory()
{
    // Create DXGI Factory
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_pDxgiFactory)));

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

    msQualityLevels.Format              = DXGI_FORMAT_R8G8_UNORM; 
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
}

// --------------------------------------------------------------------------------------------------------------------------
// discrads old swapchain and creates new one 
// Todo: needs to be a little bit more reusable for later 

void cDirectX12::InitializeSwapChain()
{
    m_pSwapChain.Reset();

    DXGI_SWAP_CHAIN_DESC sd;

    sd.BufferDesc.Width                 = m_pWindow->GetWidth();
    sd.BufferDesc.Height                = m_pWindow->GetHeight();
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.Format                = m_backBufferFormat;
    sd.BufferDesc.ScanlineOrdering      = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling               = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SampleDesc.Count                 = m_4xMsaaQuality ? 4 : 1;
    sd.SampleDesc.Quality               = m_4xMsaaQuality ? (m_4xMsaaQuality - 1) : 0;
    sd.BufferUsage                      = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount                      = c_swapChainBufferCount;
    sd.OutputWindow                     = m_pWindow->GetHWND();
    sd.Windowed                         = true;
    sd.SwapEffect                       = DXGI_SWAP_EFFECT_DISCARD;                        
    sd.Flags                            = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;           // swap chain uses queue to perform flush

    ThrowIfFailed(m_pDxgiFactory->CreateSwapChain(
        m_pCommandQueue.Get(),
        &sd,
        m_pSwapChain.GetAddressOf()
    ));
}

// --------------------------------------------------------------------------------------------------------------------------

void cDirectX12::InitializeDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHD; 

    rtvHD.NumDescriptors    = c_swapChainBufferCount;
    rtvHD.Type              = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHD.Flags             = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHD.NodeMask          = 0;

    ThrowIfFailed(m_pDevice->CreateDescriptorHeap(
        &rtvHD,
        IID_PPV_ARGS(m_pRtvHeap.GetAddressOf())
    ));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHD;

    dsvHD.NumDescriptors    = 1;
    dsvHD.Type              = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHD.Flags             = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHD.NodeMask          = 0;

    ThrowIfFailed(m_pDevice->CreateDescriptorHeap(
        &dsvHD,
        IID_PPV_ARGS(m_pDsvHeap.GetAddressOf())
    ));

}

// --------------------------------------------------------------------------------------------------------------------------
