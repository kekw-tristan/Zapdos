#include "gpuTexture.h"

#include <vector>
#include <d3dx12.h>

#include "cpuTexture.h"

// --------------------------------------------------------------------------------------------------------------------------

cGpuTexture::cGpuTexture()
	: m_pTexture(nullptr)
	, m_pUploadheap(nullptr)
	, m_srvCpuHandle()
	, m_srvGpuHandle()
{
}

// --------------------------------------------------------------------------------------------------------------------------

cGpuTexture::~cGpuTexture()
{
  
}

// --------------------------------------------------------------------------------------------------------------------------

void cGpuTexture::UploadToGpu(cCpuTexture& _rCpuTexture, ID3D12Device* _pDevice, ID3D12GraphicsCommandList* _pCmdList)
{
    const uint8_t* pData = _rCpuTexture.GetData().data();
    DXGI_FORMAT format = _rCpuTexture.GetFormat();
    int width = _rCpuTexture.GetWidth();
    int height = _rCpuTexture.GetHeight();

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    CD3DX12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);

    _pDevice->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_pTexture)
    );

    // -------------------------
    // 2. UPLOAD BUFFER
    // -------------------------
    UINT64 uploadSize = 0;

    _pDevice->GetCopyableFootprints(
        &desc,
        0, 1, 0,
        nullptr,
        nullptr,
        nullptr,
        &uploadSize
    );

    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

    _pDevice->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_pUploadheap)
    );

    D3D12_SUBRESOURCE_DATA sub = {};
    sub.pData = pData;

    sub.RowPitch = width * 4;
    sub.SlicePitch = sub.RowPitch * height;

    UpdateSubresources(
        _pCmdList,
        m_pTexture.Get(),
        m_pUploadheap.Get(),
        0, 0, 1,
        &sub
    );

    _pCmdList->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(
            m_pTexture.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        )
    );
}

// --------------------------------------------------------------------------------------------------------------------------

ID3D12Resource* cGpuTexture::GetResource()
{
	return m_pTexture.Get();
}

// --------------------------------------------------------------------------------------------------------------------------

void cGpuTexture::ReleaseUploadHeap()
{
    m_pUploadheap.Reset();
}


// --------------------------------------------------------------------------------------------------------------------------