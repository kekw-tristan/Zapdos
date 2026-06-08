#include "TextureManager.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

#include "cpuTexture.h"
#include "commandContext.h"
#include "gfxConfig.h"

#include "d3dx12.h"

// --------------------------------------------------------------------------------------------------------------------------

int cTextureManager::UploadCpuTextures(std::vector<cCpuTexture>& _rCpuTextures, ID3D12Device* _pDevice, ID3D12DescriptorHeap* _pHeap,
    cCommandContext* _pCommandContext, ID3D12PipelineState* _pMipGenPipelineState,
    ID3D12RootSignature* _pMipGenRootSignature, UINT _textureSrvBaseOffset
)
{
    assert(_pDevice);
    assert(_pHeap);
    assert(_pCommandContext);
    assert(_pMipGenPipelineState);
    assert(_pMipGenRootSignature);

    ID3D12GraphicsCommandList* pCmdList = _pCommandContext->GetCommandList();

    assert(pCmdList);

    const UINT descriptorSize   = _pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const UINT numTextures      = min(static_cast<UINT>(_rCpuTextures.size()), GFX_MAX_NUMGER_OF_TEXTURES);
    const UINT mipUavBaseOffset = _textureSrvBaseOffset + GFX_MAX_NUMGER_OF_TEXTURES;

    m_textures.clear();
    m_textures.resize(numTextures);

    ID3D12DescriptorHeap* heaps[] = { _pHeap };
    pCmdList->SetDescriptorHeaps(_countof(heaps), heaps);

    for (UINT i = 0; i < numTextures; ++i)
    {
        // ---------------------------------------------------------
        // upload texture (mip 0)
        // ---------------------------------------------------------
        m_textures[i].UploadToGpu(
            _rCpuTextures[i],
            _pDevice,
            pCmdList
        );

        ID3D12Resource* pTexture =
            m_textures[i].GetResource();

        assert(pTexture);

        // ---------------------------------------------------------
        // create SRVs
        // ---------------------------------------------------------
        const UINT srvHeapIndex = _textureSrvBaseOffset + i;

        D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = CreateSRV(_pDevice, _pHeap, pTexture, descriptorSize, srvHeapIndex);

        // ---------------------------------------------------------
        // create UAVs for mipmaps
        // ---------------------------------------------------------
        std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> uavGpuHandles = 
            CreateMipUAVs(_pDevice, _pHeap, pTexture, descriptorSize, i, mipUavBaseOffset, GFX_MAX_MIP_MAPS_PER_TEXTURE);

        // ---------------------------------------------------------
        // generate mipmaps (compute shader)
        // ---------------------------------------------------------
        GenerateMipmaps(pCmdList, pTexture, srvGpuHandle, uavGpuHandles, _pMipGenPipelineState, _pMipGenRootSignature);
    }

    return static_cast<int>(numTextures);
}

// --------------------------------------------------------------------------------------------------------------------------

void cTextureManager::ReleaseUploadHeaps()
{
    for (cGpuTexture& texture : m_textures)
    {
        texture.ReleaseUploadHeap();
    }
}

// --------------------------------------------------------------------------------------------------------------------------

D3D12_CPU_DESCRIPTOR_HANDLE cTextureManager::GetCpuHandle(ID3D12DescriptorHeap* _pHeap, UINT _descriptorSize, UINT _heapIndex) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = _pHeap->GetCPUDescriptorHandleForHeapStart();

    handle.ptr += static_cast<SIZE_T>(_heapIndex) * _descriptorSize;

    return handle;
}

// --------------------------------------------------------------------------------------------------------------------------

D3D12_GPU_DESCRIPTOR_HANDLE cTextureManager::GetGpuHandle(ID3D12DescriptorHeap* _pHeap, UINT _descriptorSize, UINT _heapIndex) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = _pHeap->GetGPUDescriptorHandleForHeapStart();

    handle.ptr += static_cast<UINT64>(_heapIndex) * _descriptorSize;

    return handle;
}

// --------------------------------------------------------------------------------------------------------------------------

D3D12_GPU_DESCRIPTOR_HANDLE cTextureManager::CreateSRV(ID3D12Device* _pDevice, ID3D12DescriptorHeap* _pHeap, 
    ID3D12Resource* _pTexture, UINT _descriptorSize, UINT _heapIndex) const
{
    const D3D12_RESOURCE_DESC texDesc = _pTexture->GetDesc();

    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = GetCpuHandle(_pHeap, _descriptorSize, _heapIndex);
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = GetGpuHandle(_pHeap, _descriptorSize, _heapIndex);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

    srvDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format                        = texDesc.Format;
    srvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip     = 0;
    srvDesc.Texture2D.MipLevels           = texDesc.MipLevels;
    srvDesc.Texture2D.PlaneSlice          = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    _pDevice->CreateShaderResourceView(_pTexture, &srvDesc, srvCpuHandle);

    return srvGpuHandle;
}

// --------------------------------------------------------------------------------------------------------------------------

std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> cTextureManager::CreateMipUAVs(ID3D12Device* _pDevice, ID3D12DescriptorHeap* _pHeap, ID3D12Resource* _pTexture,
    UINT _descriptorSize, UINT _textureIndex, UINT _mipUavBaseOffset, UINT _maxMipLevelsPerTexture) const
{
    const D3D12_RESOURCE_DESC texDesc = _pTexture->GetDesc();

    const UINT mipLevels = texDesc.MipLevels;

    if (mipLevels > _maxMipLevelsPerTexture)
    {
        throw std::runtime_error(
            "Texture has more mip levels than the reserved UAV descriptor block."
        );
    }

    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> uavGpuHandles;
    uavGpuHandles.resize(mipLevels);

    // mip 0 already exists
    for (UINT mip = 1; mip < mipLevels; ++mip)
    {
        const UINT uavHeapIndex = _mipUavBaseOffset + _textureIndex * _maxMipLevelsPerTexture + mip;

        D3D12_CPU_DESCRIPTOR_HANDLE uavCpuHandle = GetCpuHandle(_pHeap, _descriptorSize, uavHeapIndex);
        D3D12_GPU_DESCRIPTOR_HANDLE uavGpuHandle = GetGpuHandle(_pHeap, _descriptorSize, uavHeapIndex);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        
        uavDesc.Format               = texDesc.Format;
        uavDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice   = mip;
        uavDesc.Texture2D.PlaneSlice = 0;

        _pDevice->CreateUnorderedAccessView(_pTexture, nullptr, &uavDesc, uavCpuHandle);

        uavGpuHandles[mip] = uavGpuHandle;
    }

    return uavGpuHandles;
}

// --------------------------------------------------------------------------------------------------------------------------

void cTextureManager::GenerateMipmaps(ID3D12GraphicsCommandList* _pCmdList, ID3D12Resource* _pTexture, D3D12_GPU_DESCRIPTOR_HANDLE _srvGpuHandle,
    const std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>& _uavGpuHandles, ID3D12PipelineState* _pMipGenPipelineState, ID3D12RootSignature* _pMipGenRootSignature) const
{
    const D3D12_RESOURCE_DESC desc = _pTexture->GetDesc();

    const UINT mipLevels = desc.MipLevels;

    if (mipLevels <= 1)
        return;

    _pCmdList->SetPipelineState(_pMipGenPipelineState);
    _pCmdList->SetComputeRootSignature(_pMipGenRootSignature);

    // Mip 0: COPY_DEST -> NON_PIXEL_SHADER_RESOURCE
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            _pTexture,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            0
        );

        _pCmdList->ResourceBarrier(1, &barrier);
    }

    for (UINT mip = 1; mip < mipLevels; ++mip)
    {
        // target-mip: COPY_DEST -> UNORDERED_ACCESS
        {
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                _pTexture,
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                mip
            );

            _pCmdList->ResourceBarrier(1, &barrier);
        }

        _pCmdList->SetComputeRootDescriptorTable(0, _srvGpuHandle);
        _pCmdList->SetComputeRootDescriptorTable(1, _uavGpuHandles[mip]);

        // Root Constant: source mip
        _pCmdList->SetComputeRoot32BitConstant(
            2,
            mip - 1,
            0
        );

        const UINT dstWidth =
            std::max<UINT>(
                1,
                static_cast<UINT>(desc.Width >> mip)
            );

        const UINT dstHeight =
            std::max<UINT>(
                1,
                desc.Height >> mip
            );

        _pCmdList->Dispatch(
            (dstWidth + 7) / 8,
            (dstHeight + 7) / 8,
            1
        );

        auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(_pTexture);


        // current mip as next source
        _pCmdList->ResourceBarrier(1, &uavBarrier);
        {
            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                _pTexture,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                mip
            );

            _pCmdList->ResourceBarrier(1, &barrier);
        }
    }

    // all mips to render
    for (UINT mip = 0; mip < mipLevels; ++mip)
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            _pTexture,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            mip
        );

        _pCmdList->ResourceBarrier(1, &barrier);
    }
}

// --------------------------------------------------------------------------------------------------------------------------