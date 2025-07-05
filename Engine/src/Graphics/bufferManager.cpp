#include "bufferManager.h"

#include <iostream>

#include "deviceManager.h"
#include "directx12Util.h"
#include "uploadBuffer.h"
#include "directx12.h"

// --------------------------------------------------------------------------------------------------------------------------

cBufferManager::cBufferManager(cDeviceManager* _pDeviceManager, cSwapChainManager* _pSwapChainManager)
    : m_pDeviceManager(_pDeviceManager)
    , m_pSwapChainManager(_pSwapChainManager)
    , m_maxNumberOfRenderItems(0)
    , m_maxNumberOfLights(0)
{
}

// --------------------------------------------------------------------------------------------------------------------------

cBufferManager::~cBufferManager()
{
}

// --------------------------------------------------------------------------------------------------------------------------

void cBufferManager::Initialize(unsigned int _maxNumberOfRenderItems, unsigned int _maxNumberOfLights)
{
    m_maxNumberOfRenderItems    = _maxNumberOfRenderItems;
    m_maxNumberOfLights         = _maxNumberOfLights;

    InitializeDescriptorHeaps();
}

// --------------------------------------------------------------------------------------------------------------------------

ID3D12DescriptorHeap* cBufferManager::GetCbvHeap() const
{
    return m_pCbvHeap.Get();
}

// --------------------------------------------------------------------------------------------------------------------------

void cBufferManager::InitializeDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};

    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;  
    std::cout << "lol: " << m_maxNumberOfLights << std::endl;
    cbvHeapDesc.NumDescriptors = c_NumberOfFrameResources * (m_maxNumberOfRenderItems + m_maxNumberOfLights + 1); // 1 is for passconstants
    cbvHeapDesc.NodeMask = 0;

    cDirectX12Util::ThrowIfFailed(m_pDeviceManager->GetDevice()->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_pCbvHeap)));
}

// --------------------------------------------------------------------------------------------------------------------------
