#include "bufferManager.h"

#include "deviceManager.h"
#include "directx12Util.h"
#include "uploadBuffer.h"

// --------------------------------------------------------------------------------------------------------------------------

cBufferManager::cBufferManager(cDeviceManager* _pDeviceManager, cSwapChainManager* _pSwapChainManager)
    : m_pDeviceManager(_pDeviceManager)
    , m_pSwapChainManager(_pSwapChainManager)
    , m_pObjectCB(nullptr)
{
}

// --------------------------------------------------------------------------------------------------------------------------

void cBufferManager::Initialize()
{
    InitializeDescriptorHeaps();
    InitializeConstantBuffer();
}

// --------------------------------------------------------------------------------------------------------------------------

void cBufferManager::InitializeDescriptorHeaps()
{
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;

    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;  // Ensure this is shader visible
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.NodeMask = 0;

    cDirectX12Util::ThrowIfFailed(m_pDeviceManager->GetDevice()->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_pCbvHeap)));
}

// --------------------------------------------------------------------------------------------------------------------------

void cBufferManager::InitializeConstantBuffer()
{
    m_pObjectCB = new cUploadBuffer<sObjectConstants>(m_pDeviceManager->GetDevice(), 1, true);  // Assuming 1 object in the buffer

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
    m_pDeviceManager->GetDevice()->CreateConstantBufferView(&cbvDesc, m_pCbvHeap->GetCPUDescriptorHandleForHeapStart());
}
