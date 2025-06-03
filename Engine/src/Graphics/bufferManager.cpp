#include "bufferManager.h"

#include "deviceManager.h"
#include "directx12Util.h"
#include "uploadBuffer.h"

// --------------------------------------------------------------------------------------------------------------------------

cBufferManager::cBufferManager(cDeviceManager* _pDeviceManager, cSwapChainManager* _pSwapChainManager)
    : m_pDeviceManager(_pDeviceManager)
    , m_pSwapChainManager(_pSwapChainManager)
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

void cBufferManager::InitializeDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};

    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;  // Ensure this is shader visible
    cbvHeapDesc.NumDescriptors = 9;
    cbvHeapDesc.NodeMask = 0;

    cDirectX12Util::ThrowIfFailed(m_pDeviceManager->GetDevice()->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_pCbvHeap)));
}

// --------------------------------------------------------------------------------------------------------------------------
