#include "bufferManager.h"

#include <iostream>

#include "deviceManager.h"
#include "directx12Util.h"
#include "gfxConfig.h"
#include "uploadBuffer.h"
#include "directx12.h"

// --------------------------------------------------------------------------------------------------------------------------

cBufferManager::cBufferManager(cDeviceManager* _pDeviceManager, cSwapChainManager* _pSwapChainManager)
    : m_pDeviceManager(_pDeviceManager)
    , m_pSwapChainManager(_pSwapChainManager)
    , m_textureOffset(0)
{
}

// --------------------------------------------------------------------------------------------------------------------------

cBufferManager::~cBufferManager()
{
}

// --------------------------------------------------------------------------------------------------------------------------

void cBufferManager::Initialize()
{
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
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    UINT descriptorsPerFrame = GFX_MAX_NUMBER_OF_RENDER_ITEMS + 1 + 1; // obj CBVs + pass + lights
    heapDesc.NumDescriptors = c_NumberOfFrameResources * descriptorsPerFrame + GFX_MAX_NUMGER_OF_TEXTURES;
    heapDesc.NodeMask = 0;

    m_textureOffset = c_NumberOfFrameResources * descriptorsPerFrame;

    cDirectX12Util::ThrowIfFailed(m_pDeviceManager->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_pCbvHeap)));
}

// --------------------------------------------------------------------------------------------------------------------------
