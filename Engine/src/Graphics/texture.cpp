#include "texture.h"

#include <iostream>


#include "DDSTextureLoader12.h"
#include "directx12Util.h"

// --------------------------------------------------------------------------------------------------------------------------

cTexture::cTexture(int _index, std::wstring& _rFilePath)
	: m_index(_index)
	, m_filePath(_rFilePath)
	, m_pResource()
	, m_pUploadHeap()
	, m_srvHandle()
    , m_width(0)
    , m_height(0)
    , m_mipLevels(0)
{
}

// --------------------------------------------------------------------------------------------------------------------------

void cTexture::LoadTexture(ID3D12Device* _pDevice)
{

    cDirectX12Util::ThrowIfFailed(
        DirectX::LoadDDSTextureFromFile(
            _pDevice,
            m_filePath.c_str(),
            &m_pResource,
            m_pDdsData,
            m_subresources
        )
    );

    auto desc = m_pResource->GetDesc();

    m_width     = desc.Width;
    m_height    = desc.Height;
    m_mipLevels = desc.MipLevels;

    std::wcout << "Loaded: " << m_filePath << "\n";
}

// --------------------------------------------------------------------------------------------------------------------------

void cTexture::UploadToGpu(ID3D12Device* _pDevice, ID3D12GraphicsCommandList* _pCmdList)
{

    UINT64 uploadBufferSize = 0;

    uploadBufferSize = GetRequiredIntermediateSize(
        m_pResource.Get(),
        0,
        static_cast<UINT>(m_subresources.size())
    );

    cDirectX12Util::ThrowIfFailed(
        _pDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_pUploadHeap)
        )
    );

    UpdateSubresources(
        _pCmdList,
        m_pResource.Get(),
        m_pUploadHeap.Get(),
        0,
        0,
        static_cast<UINT>(m_subresources.size()),
        m_subresources.data()
    );

}

// --------------------------------------------------------------------------------------------------------------------------


ID3D12Resource* cTexture::GetResource()
{
    return m_pResource.Get();
}

// --------------------------------------------------------------------------------------------------------------------------


int cTexture::GetIndex()
{
    return m_index;
}

// --------------------------------------------------------------------------------------------------------------------------


int cTexture::GetMipLevels()
{
    return m_mipLevels;
}

// --------------------------------------------------------------------------------------------------------------------------


std::wstring& cTexture::GetFilePath()
{
    return m_filePath;
}

// --------------------------------------------------------------------------------------------------------------------------
