#include "texture.h"

#include <iostream>
#include <memory>

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
    std::unique_ptr<uint8_t[]> ddsData;
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;

    cDirectX12Util::ThrowIfFailed(
        DirectX::LoadDDSTextureFromFile(
            _pDevice,
            m_filePath.c_str(),
            &m_pResource,
            ddsData,
            subresources
        )
    );

    auto desc = m_pResource->GetDesc();

    m_width     = desc.Width;
    m_height    = desc.Height;
    m_mipLevels = desc.MipLevels;

    std::wcout << "Loaded: " << m_filePath << "\n";
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
