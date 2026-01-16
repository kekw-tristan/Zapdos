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
    , m_maxNumberOfTextures(7)
    , m_textureOffset(0)
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

int cBufferManager::GetTextureOffset()
{
    return m_textureOffset;
}

// --------------------------------------------------------------------------------------------------------------------------

void cBufferManager::InitializeDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};

    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;  
    UINT descriptorsPerFrame = m_maxNumberOfRenderItems + 1 + 1; // obj CBVs + pass + lights
    cbvHeapDesc.NumDescriptors = c_NumberOfFrameResources * descriptorsPerFrame + m_maxNumberOfTextures;
    cbvHeapDesc.NodeMask = 0;

    m_textureOffset = c_NumberOfFrameResources * descriptorsPerFrame;

    cDirectX12Util::ThrowIfFailed(m_pDeviceManager->GetDevice()->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_pCbvHeap)));
}

// --------------------------------------------------------------------------------------------------------------------------
