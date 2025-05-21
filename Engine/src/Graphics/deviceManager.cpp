#include "deviceManager.h"

#include <iostream>

#include "directx12Util.h"

// --------------------------------------------------------------------------------------------------------------------------
// Constructor

cDeviceManager::cDeviceManager()
		:	m_pDevice		    (nullptr)
		,	m_pDxgiFactory	    (nullptr)
		,	m_pCommandQueue	    (nullptr)
		,	m_pFence		    (nullptr)
		,	m_pFenceEvent	    (nullptr)
		,	m_fenceValue	    (0)
        ,   m_4xMsaaQuality     (0)
        ,   m_currentFence      (0)
        ,   m_descriptorSizes   ()
{
}

// --------------------------------------------------------------------------------------------------------------------------
// Initialized all the components

void cDeviceManager::Initialize()
{
    InitializeDeviceAndFactory();
    InitializeFenceAndDescriptorSize();
    Check4XMSAAQualitySupport();
    InitializeCommandQueueAndList();

}

// --------------------------------------------------------------------------------------------------------------------------

void cDeviceManager::FlushCommandQueue()
{
    // Increment the fence value to mark the current set of GPU commands.
    m_currentFence++;

    // Signal the fence with the current fence value. This tells the GPU to set the fence to this value when it has finished processing all commands up to this point.
    cDirectX12Util::ThrowIfFailed(m_pCommandQueue->Signal(m_pFence.Get(), m_currentFence));

    // If the GPU has not yet reached the current fence value, wait for it.
    if (m_pFence->GetCompletedValue() < m_currentFence)
    {
        // Create an event handle for GPU synchronization.
        HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        // Instruct the fence to trigger the event when it reaches the current fence value.
        cDirectX12Util::ThrowIfFailed(m_pFence->SetEventOnCompletion(m_currentFence, eventHandle));
        
        if (eventHandle == nullptr)
        {
            // Handle the error — for example, log it and/or throw
            throw std::runtime_error("Failed to create event handle.");
        }


        // Wait for the GPU to complete execution up to the current fence value.
        WaitForSingleObject(eventHandle, INFINITE);

        // Clean up the event handle.
        CloseHandle(eventHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------------

ID3D12Device* cDeviceManager::GetDevice() const
{
	return m_pDevice.Get();
}

// --------------------------------------------------------------------------------------------------------------------------

IDXGIFactory7* cDeviceManager::GetDxgiFactory() const
{
	return m_pDxgiFactory.Get();
}

// --------------------------------------------------------------------------------------------------------------------------

ID3D12CommandQueue* cDeviceManager::GetCommandQueue() const
{
	return m_pCommandQueue.Get();
}

// --------------------------------------------------------------------------------------------------------------------------

ID3D12Fence* cDeviceManager::GetFence() const
{
    return m_pFence.Get();
}

ID3D12GraphicsCommandList* cDeviceManager::GetCommandList() const
{
    return m_pCommandList.Get();
}

ID3D12CommandAllocator* cDeviceManager::GetDirectCmdListAlloc() const
{
    return m_pDirectCmdListAlloc.Get();
}

// --------------------------------------------------------------------------------------------------------------------------

const cDeviceManager::sDescriptorSizes& cDeviceManager::GetDescriptorSizes() const
{
    return m_descriptorSizes;
}

// --------------------------------------------------------------------------------------------------------------------------

int cDeviceManager::Get4xMSAAQuality() const
{
    return m_4xMsaaQuality;
}

// --------------------------------------------------------------------------------------------------------------------------
// Initializes Device and Factory

void cDeviceManager::InitializeDeviceAndFactory()
{
    // Create DXGI Factory
    cDirectX12Util::ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&m_pDxgiFactory)));

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

        cDirectX12Util::ThrowIfFailed(m_pDxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));


        cDirectX12Util::ThrowIfFailed(D3D12CreateDevice(
            pWarpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_pDevice)
        ));
    }
}

// --------------------------------------------------------------------------------------------------------------------------
// Initializes Fence and rtv, dsv, cbvSrvUav, sampler descriptor sizes

void cDeviceManager::InitializeFenceAndDescriptorSize()
{
    // creating fence
    cDirectX12Util::ThrowIfFailed(m_pDevice->CreateFence(
        0,                          // initial value
        D3D12_FENCE_FLAG_NONE,      // flags 
        IID_PPV_ARGS(&m_pFence)     // fence output
    ));

    // get descriptor sizes
    m_descriptorSizes.rtv       = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_descriptorSizes.dsv       = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_descriptorSizes.cbvSrvUav = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_descriptorSizes.sampler   = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

// --------------------------------------------------------------------------------------------------------------------------
// Checks the 4xmsaa quality support level

void cDeviceManager::Check4XMSAAQualitySupport()
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels = {};

    msQualityLevels.Format = c_backBufferFormat;
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
}

// --------------------------------------------------------------------------------------------------------------------------
// Initializes command queue and command list

void cDeviceManager::InitializeCommandQueueAndList()
{
    // creating command queue 
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};

    queueDesc.Type  = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    cDirectX12Util::ThrowIfFailed(m_pDevice->CreateCommandQueue(
        &queueDesc,
        IID_PPV_ARGS(&m_pCommandQueue)
    ));

    // creating direct command list allocator
    cDirectX12Util::ThrowIfFailed(m_pDevice->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(m_pDirectCmdListAlloc.GetAddressOf())
    ));

    // creating command list 
    cDirectX12Util::ThrowIfFailed(m_pDevice->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_pDirectCmdListAlloc.Get(),                    // associated command allocator
        nullptr,                                        // initial pipeline state object
        IID_PPV_ARGS(m_pCommandList.GetAddressOf())
    ));
}

// --------------------------------------------------------------------------------------------------------------------------

